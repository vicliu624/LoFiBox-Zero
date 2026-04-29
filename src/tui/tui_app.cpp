// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/tui_app.h"

#include <chrono>
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "platform/tui/terminal_capabilities.h"
#include "platform/tui/terminal_input.h"
#include "platform/tui/terminal_presenter.h"
#include "runtime/unix_socket_runtime_transport.h"
#include "tui/tui_input_router.h"
#include "tui/tui_model.h"
#include "tui/tui_renderer.h"

namespace lofibox::tui {
namespace {

struct ParsedTuiArgs {
    TuiOptions options{};
    bool help{false};
};

std::atomic_bool g_signal_requested{false};

void handleTuiSignal(int) noexcept
{
    g_signal_requested = true;
}

bool isOptionWithValue(std::string_view option)
{
    return option == "--charset"
        || option == "--theme"
        || option == "--runtime-socket"
        || option == "--layout"
        || option == "--view"
        || option == "--refresh";
}

std::optional<TuiLayoutKind> layoutFromName(std::string_view value)
{
    if (value == "tiny") return TuiLayoutKind::Tiny;
    if (value == "micro") return TuiLayoutKind::Micro;
    if (value == "compact") return TuiLayoutKind::Compact;
    if (value == "normal") return TuiLayoutKind::Normal;
    if (value == "wide") return TuiLayoutKind::Wide;
    return std::nullopt;
}

std::chrono::milliseconds parseRefresh(std::string_view value)
{
    std::string text{value};
    if (text.size() > 2 && text.substr(text.size() - 2) == "ms") {
        text.resize(text.size() - 2);
    }
    try {
        return std::chrono::milliseconds{std::max(16, std::stoi(text))};
    } catch (...) {
        return std::chrono::milliseconds{100};
    }
}

ParsedTuiArgs parseTuiArgs(int argc, char** argv, bool subcommand)
{
    ParsedTuiArgs parsed{};
    const auto capabilities = platform::tui::detectTerminalCapabilities();
    parsed.options.charset = capabilities.preferred_charset;
    parsed.options.color = capabilities.color;
    int index = 1;
    if (subcommand && index < argc && std::string_view{argv[index]} == "tui") {
        ++index;
    }
    for (; index < argc; ++index) {
        const std::string_view current{argv[index]};
        if (current == "--help" || current == "-h") {
            parsed.help = true;
            continue;
        }
        if (current == "--once") {
            parsed.options.once = true;
            continue;
        }
        if (current == "--no-color") {
            parsed.options.color = false;
            parsed.options.theme = TuiTheme::Mono;
            continue;
        }
        if (isOptionWithValue(current) && index + 1 < argc) {
            const std::string_view value{argv[++index]};
            if (current == "--charset") parsed.options.charset = tuiCharsetFromName(value, parsed.options.charset);
            else if (current == "--theme") parsed.options.theme = tuiThemeFromName(value, parsed.options.theme);
            else if (current == "--runtime-socket") parsed.options.runtime_socket = std::string{value};
            else if (current == "--view") parsed.options.initial_view = tuiViewModeFromName(value);
            else if (current == "--layout") parsed.options.layout_override = layoutFromName(value);
            else if (current == "--refresh") parsed.options.refresh = parseRefresh(value);
            continue;
        }
        if (!current.empty() && current.front() != '-') {
            parsed.options.initial_view = tuiViewModeFromName(current);
        }
    }
    if (const char* env_charset = std::getenv("LOFIBOX_TUI_CHARSET"); env_charset != nullptr && *env_charset != '\0') {
        parsed.options.charset = tuiCharsetFromName(env_charset, parsed.options.charset);
    }
    return parsed;
}

void printTuiHelp(std::ostream& out)
{
    out << "Usage:\n"
        << "  lofibox-tui [view] [options]\n"
        << "  lofibox tui [view] [options]\n\n"
        << "Views: dashboard, now, lyrics, spectrum, queue, library, sources, eq, diagnostics, creator\n"
        << "Options:\n"
        << "  --charset unicode|ascii|minimal\n"
        << "  --theme dark|light|amber|mono\n"
        << "  --no-color\n"
        << "  --runtime-socket <path>\n"
        << "  --layout tiny|micro|compact|normal|wide\n"
        << "  --refresh <ms>\n"
        << "  --once\n";
}

class TerminalSessionGuard {
public:
    explicit TerminalSessionGuard(platform::tui::TerminalPresenter& presenter, platform::tui::TerminalInput& input)
        : presenter_(presenter),
          input_(input)
    {
        previous_int_ = std::signal(SIGINT, handleTuiSignal);
        previous_term_ = std::signal(SIGTERM, handleTuiSignal);
        presenter_.enterAlternateScreen();
        presenter_.hideCursor();
        input_.enterRawMode();
    }

    ~TerminalSessionGuard()
    {
        input_.leaveRawMode();
        presenter_.showCursor();
        presenter_.leaveAlternateScreen();
        presenter_.flush();
        std::signal(SIGINT, previous_int_);
        std::signal(SIGTERM, previous_term_);
    }

private:
    platform::tui::TerminalPresenter& presenter_;
    platform::tui::TerminalInput& input_;
    void (*previous_int_)(int){SIG_DFL};
    void (*previous_term_)(int){SIG_DFL};
};

void refreshSnapshot(TuiModel& model, runtime::UnixSocketRuntimeCommandClient& client)
{
    const auto snapshot = client.query(runtime::RuntimeQuery{runtime::RuntimeQueryKind::FullSnapshot, runtime::CommandOrigin::Automation, "tui:snapshot"});
    if (!client.lastError().empty()) {
        markTuiDisconnected(model, client.lastError());
        return;
    }
    updateTuiSnapshot(model, snapshot);
}

void applyRuntimeEvent(TuiModel& model, const runtime::RuntimeEvent& event)
{
    if (event.kind == runtime::RuntimeEventKind::RuntimeDisconnected) {
        markTuiDisconnected(model, event.message.empty() ? "Runtime event stream disconnected." : event.message);
        return;
    }
    updateTuiSnapshot(model, event.snapshot);
}

class RuntimeEventIngestThread {
public:
    explicit RuntimeEventIngestThread(std::filesystem::path socket_path)
        : socket_path_(std::move(socket_path))
    {
    }

    ~RuntimeEventIngestThread()
    {
        stop();
    }

    void start()
    {
        running_ = true;
        thread_ = std::thread([this]() { run(); });
    }

    void stop() noexcept
    {
        running_ = false;
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    std::vector<runtime::RuntimeEvent> drainEvents()
    {
        const std::lock_guard lock{mutex_};
        auto events = std::move(events_);
        events_.clear();
        return events;
    }

    std::string lastError() const
    {
        const std::lock_guard lock{mutex_};
        return last_error_;
    }

private:
    void setError(std::string value)
    {
        const std::lock_guard lock{mutex_};
        last_error_ = std::move(value);
    }

    void pushEvent(runtime::RuntimeEvent event)
    {
        const std::lock_guard lock{mutex_};
        events_.push_back(std::move(event));
        if (events_.size() > 256) {
            events_.erase(events_.begin(), events_.begin() + static_cast<std::ptrdiff_t>(events_.size() - 256));
        }
    }

    void run()
    {
        while (running_) {
            runtime::UnixSocketRuntimeEventStream stream{socket_path_};
            runtime::RuntimeEventStreamRequest request{};
            request.query = runtime::RuntimeQuery{runtime::RuntimeQueryKind::FullSnapshot, runtime::CommandOrigin::Automation, "tui:event-stream"};
            request.heartbeat_ms = 1000;
            if (!stream.connect(request)) {
                setError(stream.lastError());
                std::this_thread::sleep_for(std::chrono::seconds{1});
                continue;
            }
            setError({});
            while (running_) {
                if (auto event = stream.next(std::chrono::milliseconds{250})) {
                    pushEvent(std::move(*event));
                    continue;
                }
                if (!stream.lastError().empty()) {
                    setError(stream.lastError());
                    break;
                }
            }
            stream.close();
        }
    }

    std::filesystem::path socket_path_{};
    mutable std::mutex mutex_{};
    std::vector<runtime::RuntimeEvent> events_{};
    std::string last_error_{};
    std::atomic_bool running_{false};
    std::thread thread_{};
};

} // namespace

int runTuiAppFromArgv(int argc, char** argv, std::ostream& out, std::ostream& err)
{
    const auto parsed = parseTuiArgs(argc, argv, false);
    if (parsed.help) {
        printTuiHelp(out);
        return 0;
    }

    auto model = makeTuiModel(parsed.options);
    runtime::UnixSocketRuntimeCommandClient client{
        parsed.options.runtime_socket ? std::filesystem::path{*parsed.options.runtime_socket} : runtime::defaultRuntimeSocketPath()};
    TuiRenderer renderer{};

    if (parsed.options.once) {
        refreshSnapshot(model, client);
        out << renderer.render(model, TerminalSize{80, 24}).toString() << '\n';
        return 0;
    }

    platform::tui::TerminalPresenter presenter{out};
    platform::tui::TerminalInput input{};
    g_signal_requested = false;
    TerminalSessionGuard guard{presenter, input};
    platform::tui::TerminalDiffRenderer diff{presenter};
    RuntimeEventIngestThread events{client.socketPath()};
    events.start();
    markTuiDisconnected(model, "Connecting to runtime event stream...");

    while (!model.quit_requested && !g_signal_requested) {
        bool received_event = false;
        for (const auto& event : events.drainEvents()) {
            applyRuntimeEvent(model, event);
            received_event = true;
        }
        if (!received_event) {
            const auto error = events.lastError();
            if (!error.empty()) {
                markTuiDisconnected(model, error);
            }
        }
        diff.draw(renderer.render(model, presenter.size()));
        if (const auto event = input.poll(parsed.options.refresh)) {
            const auto action = routeTuiInput(model, *event);
            if (action.kind == TuiInputActionKind::RuntimeCommand && action.command) {
                (void)client.dispatch(*action.command);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    events.stop();
    (void)err;
    return 0;
}

std::optional<int> runTuiSubcommandFromArgv(int argc, char** argv, std::ostream& out, std::ostream& err)
{
    for (int index = 1; index < argc; ++index) {
        const std::string_view current{argv[index]};
        if (current == "--runtime-socket" || current == "--charset" || current == "--theme" || current == "--layout" || current == "--refresh" || current == "--view") {
            ++index;
            continue;
        }
        if (!current.empty() && current.front() == '-') {
            continue;
        }
        if (current == "tui") {
            const auto parsed = parseTuiArgs(argc, argv, true);
            if (parsed.help) {
                printTuiHelp(out);
                return 0;
            }
            return runTuiAppFromArgv(argc, argv, out, err);
        }
        return std::nullopt;
    }
    return std::nullopt;
}

} // namespace lofibox::tui
