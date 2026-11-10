// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <cctype>
#include <functional>
#include <iostream>
#include <mutex>
#include <stop_token>
#include <thread>
#include <utility>
#include <vector>
#include <unistd.h>

#include "app/input_event.h"
#include "core/operation_cancellation.h"
#include "platform/host/runtime_host_internal.h"

namespace lofibox::platform::host {

// A framebuffer application owns keyboard input rather than compositor key
// bindings. Keep system-volume IPC off its render/input thread, with one worker
// and bounded accumulated changes instead of one subprocess per repeat event.
class SystemMediaKeyHandler {
public:
    using Executor = std::function<bool(int, bool, std::stop_token)>;

    explicit SystemMediaKeyHandler(Executor executor = {})
        : executor_(executor ? std::move(executor) : defaultExecutor()) {}

    ~SystemMediaKeyHandler()
    {
        worker_.request_stop();
        changed_.notify_all();
        // worker_ is the last member: its jthread destructor joins before any
        // state/executor accessed by the worker is destroyed.
    }

    bool handle(const app::InputEvent& event)
    {
        using app::InputKey;
        if (event.key != InputKey::VolumeDown && event.key != InputKey::VolumeUp && event.key != InputKey::VolumeMute) return false;
        if (event.key == InputKey::VolumeMute && event.repeated) return true;
        const std::lock_guard lock{mutex_};
        if (event.key == InputKey::VolumeMute) pending_mute_ = !pending_mute_;
        else pending_steps_ = std::clamp(pending_steps_ + (event.key == InputKey::VolumeUp ? 1 : -1), -20, 20);
        if (!worker_.joinable()) worker_ = std::jthread([this](std::stop_token stop) { run(stop); });
        changed_.notify_all();
        return true;
    }

private:
    static Executor defaultExecutor()
    {
        namespace rt = runtime_detail;
        const auto* runtime_env = std::getenv("XDG_RUNTIME_DIR");
        const auto runtime = runtime_env && *runtime_env ? std::filesystem::path{runtime_env}
            : std::filesystem::path{"/run/user/" + std::to_string(::getuid())};
        const auto wpctl = rt::resolveExecutablePath("WPCTL_PATH", "wpctl");
        const auto pactl = rt::resolveExecutablePath("PACTL_PATH", "pactl");
        const auto amixer = rt::resolveExecutablePath("AMIXER_PATH", "amixer");
        std::error_code error;
        const bool pipewire = wpctl && std::filesystem::exists(runtime / "pipewire-0", error);
        const bool pulse = pactl && std::filesystem::exists(runtime / "pulse/native", error);
        return [wpctl, pactl, amixer, pipewire, pulse](int steps, bool mute, std::stop_token stop) {
            const core::OperationCancellationScope cancellation{stop};
            const auto invoke = [&](const auto& executable, const std::vector<std::string>& args) {
                return executable && !stop.stop_requested() && rt::captureProcessOutput(*executable, args).has_value();
            };
            bool success = true;
            const auto magnitude = std::to_string(std::abs(steps) * 5) + "%";
            if (pipewire) {
                if (steps) success = invoke(wpctl, {"set-volume", "-l", "1.0", "@DEFAULT_AUDIO_SINK@", magnitude + (steps > 0 ? "+" : "-")});
                if (mute && !stop.stop_requested()) success = invoke(wpctl, {"set-mute", "@DEFAULT_AUDIO_SINK@", "toggle"}) && success;
            } else if (pulse) {
                if (steps) {
                    // Clamp every channel to 100%, retaining unequal channel
                    // levels instead of flattening a user's stereo balance.
                    const auto current = rt::captureProcessOutput(*pactl, {"get-sink-volume", "@DEFAULT_SINK@"});
                    std::vector<std::string> args{"set-sink-volume", "@DEFAULT_SINK@"};
                    if (current) {
                        for (auto end = current->find('%'); end != std::string::npos; end = current->find('%', end + 1)) {
                            auto begin = end;
                            while (begin && std::isdigit(static_cast<unsigned char>((*current)[begin-1]))) --begin;
                            if (begin != end) args.push_back(std::to_string(std::clamp(std::stoi(current->substr(begin, end-begin)) + steps*5, 0, 100)) + "%");
                        }
                    }
                    success = args.size() > 2 && invoke(pactl, args);
                }
                if (mute && !stop.stop_requested()) success = invoke(pactl, {"set-sink-mute", "@DEFAULT_SINK@", "toggle"}) && success;
            } else {
                if (steps) success = invoke(amixer, {"-q", "sset", "Master", magnitude + (steps > 0 ? "+" : "-")});
                if (mute && !stop.stop_requested()) success = invoke(amixer, {"-q", "sset", "Master", "toggle"}) && success;
            }
            return success;
        };
    }

    void run(std::stop_token stop)
    {
        bool reported_failure = false;
        while (!stop.stop_requested()) {
            std::unique_lock lock{mutex_};
            if (!changed_.wait(lock, stop, [&] { return pending_steps_ != 0 || pending_mute_; })) return;
            // A short fixed batch window also coalesces a burst of alternating
            // keys. It never grows a queue or delays the GUI thread.
            changed_.wait_for(lock, stop, std::chrono::milliseconds{35}, [] { return false; });
            if (stop.stop_requested()) return;
            const int steps = std::exchange(pending_steps_, 0);
            const bool mute = std::exchange(pending_mute_, false);
            lock.unlock();
            bool success = false;
            try { success = executor_(steps, mute, stop); }
            catch (...) { /* Failure of a system audio helper must not terminate the player. */ }
            if (!success && !stop.stop_requested() && !reported_failure) std::cerr << "System volume shortcut failed.\n";
            reported_failure = !success;
        }
    }

    Executor executor_;
    std::mutex mutex_;
    std::condition_variable_any changed_;
    int pending_steps_{};
    bool pending_mute_{};
    std::jthread worker_;
};

} // namespace lofibox::platform::host
