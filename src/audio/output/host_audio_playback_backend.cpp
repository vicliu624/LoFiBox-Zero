// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/runtime_provider_factories.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "platform/host/png_canvas_loader.h"
#include "platform/host/runtime_logger.h"

namespace fs = std::filesystem;

namespace lofibox::platform::host {
namespace {
using namespace runtime_detail;

struct AudioExecutable {
    fs::path path{};
    bool windows_interop{false};
};

#if defined(__linux__)
bool runningUnderWsl()
{
    std::ifstream input("/proc/sys/kernel/osrelease");
    std::string value{};
    std::getline(input, value);
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value.find("microsoft") != std::string::npos || value.find("wsl") != std::string::npos;
}

std::optional<fs::path> resolveExecutableFromPath(const char* executable_name)
{
    return resolveExecutablePath(nullptr, executable_name);
}

bool isWindowsExecutablePath(const fs::path& path)
{
    auto extension = path.extension().string();
    for (char& ch : extension) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return extension == ".exe";
}

std::string wslPathForWindowsProcess(const fs::path& path)
{
    std::string input = pathUtf8String(path);
    if (input.rfind("/mnt/", 0) == 0 && input.size() > 6 && input[6] == '/') {
        const char drive = input[5];
        if (std::isalpha(static_cast<unsigned char>(drive)) != 0) {
            std::string output{};
            output.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(drive))));
            output += ':';
            for (std::size_t i = 6; i < input.size(); ++i) {
                output.push_back(input[i] == '/' ? '\\' : input[i]);
            }
            return output;
        }
    }

    if (!input.empty() && input.front() == '/') {
        const char* distro = std::getenv("WSL_DISTRO_NAME");
        if (distro != nullptr && *distro != '\0') {
            std::string output = "\\\\wsl.localhost\\";
            output += distro;
            for (const char ch : input) {
                output.push_back(ch == '/' ? '\\' : ch);
            }
            return output;
        }
    }

    return input;
}
#endif

AudioExecutable resolveAudioExecutable()
{
#if defined(_WIN32)
    if (auto path = resolveExecutablePath(L"FFPLAY_PATH", L"ffplay.exe")) {
        return AudioExecutable{*path, false};
    }
#elif defined(__linux__)
    if (const char* env_path = std::getenv("FFPLAY_PATH"); env_path != nullptr && *env_path != '\0') {
        fs::path path{env_path};
        if (fs::exists(path)) {
            return AudioExecutable{path, isWindowsExecutablePath(path)};
        }
    }

    if (auto path = resolveExecutableFromPath("ffplay")) {
        return AudioExecutable{*path, false};
    }

    if (runningUnderWsl()) {
        if (auto windows_ffplay = resolveExecutableFromPath("ffplay.exe")) {
            logRuntime(RuntimeLogLevel::Info, "audio", "Using Windows ffplay fallback for WSL audio");
            return AudioExecutable{*windows_ffplay, true};
        }
    }
#endif
    return {};
}

AudioExecutable resolveAnalyzerExecutable()
{
#if defined(_WIN32)
    if (auto path = resolveExecutablePath(L"FFMPEG_PATH", L"ffmpeg.exe")) {
        return AudioExecutable{*path, false};
    }
#elif defined(__linux__)
    if (const char* env_path = std::getenv("FFMPEG_PATH"); env_path != nullptr && *env_path != '\0') {
        fs::path path{env_path};
        if (fs::exists(path)) {
            return AudioExecutable{path, isWindowsExecutablePath(path)};
        }
    }

    if (auto path = resolveExecutableFromPath("ffmpeg")) {
        return AudioExecutable{*path, false};
    }

    if (runningUnderWsl()) {
        if (auto windows_ffmpeg = resolveExecutableFromPath("ffmpeg.exe")) {
            logRuntime(RuntimeLogLevel::Info, "audio", "Using Windows ffmpeg fallback for WSL visualization");
            return AudioExecutable{*windows_ffmpeg, true};
        }
    }
#endif
    return {};
}

bool networkInput(std::string_view input) noexcept
{
    return input.find("://") != std::string_view::npos;
}

constexpr std::chrono::seconds kNetworkPlaybackUiStartDelay{24};
constexpr std::chrono::seconds kNetworkAnalyzerWarmupLimit{5};

app::AudioVisualizationFrame spectrumFrameFromPcmWindow(const std::vector<float>& samples)
{
    constexpr double kPi = 3.14159265358979323846;
    constexpr double kSampleRate = 16000.0;
    constexpr std::array<double, 10> kBandCenters{
        63.0,
        125.0,
        250.0,
        500.0,
        1000.0,
        2000.0,
        3000.0,
        4000.0,
        5500.0,
        7000.0,
    };

    app::AudioVisualizationFrame frame{};
    if (samples.empty()) {
        return frame;
    }

    frame.available = true;
    const double n = static_cast<double>(samples.size());
    for (std::size_t band = 0; band < kBandCenters.size(); ++band) {
        const double k = std::round((n * kBandCenters[band]) / kSampleRate);
        const double omega = (2.0 * kPi * k) / n;
        const double coeff = 2.0 * std::cos(omega);
        double q0 = 0.0;
        double q1 = 0.0;
        double q2 = 0.0;
        for (std::size_t index = 0; index < samples.size(); ++index) {
            const double window = 0.5 - (0.5 * std::cos((2.0 * kPi * static_cast<double>(index)) / std::max(1.0, n - 1.0)));
            q0 = (coeff * q1) - q2 + (static_cast<double>(samples[index]) * window);
            q2 = q1;
            q1 = q0;
        }
        const double magnitude = std::sqrt(std::max(0.0, (q1 * q1) + (q2 * q2) - (q1 * q2 * coeff))) / n;
        const double compressed = std::pow(std::log1p(magnitude * 96.0) / std::log1p(96.0), 0.68);
        frame.bands[band] = static_cast<float>(std::clamp(compressed, 0.0, 1.0));
    }
    return frame;
}

class RealtimePcmSpectrumAnalyzer {
public:
    ~RealtimePcmSpectrumAnalyzer()
    {
        stop();
    }

    void start(const AudioExecutable& executable, const std::string& input, double start_seconds)
    {
        stop();
        {
            std::lock_guard lock(frame_mutex_);
            frame_ = {};
        }
        if (executable.path.empty() || input.empty()) {
            logRuntime(RuntimeLogLevel::Debug, "audio", "Visualization unavailable: ffmpeg not found");
            return;
        }

        std::vector<std::string> args{
            "-hide_banner",
            "-loglevel",
            "error",
            "-nostdin",
            "-re"};
        if (networkInput(input)) {
            args.emplace_back("-fflags");
            args.emplace_back("nobuffer");
            args.emplace_back("-flags");
            args.emplace_back("low_delay");
            args.emplace_back("-probesize");
            args.emplace_back("32768");
            args.emplace_back("-analyzeduration");
            args.emplace_back("0");
        }
        if (start_seconds > 0.0) {
            const auto seek_ms = static_cast<int>(std::floor(start_seconds * 1000.0));
            const int whole = seek_ms / 1000;
            const int frac = std::abs(seek_ms % 1000);
            char buffer[32]{};
            std::snprintf(buffer, sizeof(buffer), "%d.%03d", whole, frac);
            args.emplace_back("-ss");
            args.emplace_back(buffer);
        }
        args.emplace_back("-i");
        args.emplace_back(input);
        args.emplace_back("-vn");
        args.emplace_back("-ac");
        args.emplace_back("1");
        args.emplace_back("-ar");
        args.emplace_back("16000");
        args.emplace_back("-f");
        args.emplace_back("s16le");
        args.emplace_back("-");

        stop_requested_ = false;
        worker_ = std::thread([this, executable_path = executable.path, args = std::move(args)]() mutable {
            run(executable_path, std::move(args));
        });
    }

    void pause() noexcept
    {
        pausePipeProcess(process_);
    }

    void resume() noexcept
    {
        resumePipeProcess(process_);
    }

    void stop()
    {
        stop_requested_ = true;
        stopPipeProcess(process_);
        if (worker_.joinable()) {
            worker_.join();
        }
        {
            std::lock_guard lock(frame_mutex_);
            frame_ = {};
        }
        stop_requested_ = false;
    }

    [[nodiscard]] app::AudioVisualizationFrame currentFrame() const
    {
        std::lock_guard lock(frame_mutex_);
        return frame_;
    }

private:
    void run(const fs::path& executable, std::vector<std::string> args)
    {
        if (!spawnPipeProcess(process_, executable, args)) {
            return;
        }

        std::array<char, 4096> buffer{};
        std::vector<float> window{};
        window.reserve(kWindowSize);
        std::optional<unsigned char> carry{};
        while (!stop_requested_) {
            const int read_bytes = readPipeProcess(process_, buffer.data(), static_cast<int>(buffer.size()));
            if (read_bytes < 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds{10});
                continue;
            }
            if (read_bytes == 0) {
                break;
            }

            int offset = 0;
            if (carry && read_bytes > 0) {
                const auto low = static_cast<unsigned int>(*carry);
                const auto high = static_cast<unsigned int>(static_cast<unsigned char>(buffer[0]));
                appendSample(window, static_cast<std::int16_t>(low | (high << 8U)));
                offset = 1;
                carry.reset();
            }

            for (; offset + 1 < read_bytes; offset += 2) {
                const auto low = static_cast<unsigned int>(static_cast<unsigned char>(buffer[static_cast<std::size_t>(offset)]));
                const auto high = static_cast<unsigned int>(static_cast<unsigned char>(buffer[static_cast<std::size_t>(offset + 1)]));
                appendSample(window, static_cast<std::int16_t>(low | (high << 8U)));
            }
            if (offset < read_bytes) {
                carry = static_cast<unsigned char>(buffer[static_cast<std::size_t>(offset)]);
            }
        }
        stopPipeProcess(process_);
    }

    void appendSample(std::vector<float>& window, std::int16_t sample)
    {
        window.push_back(static_cast<float>(sample) / 32768.0f);
        if (window.size() < kWindowSize) {
            return;
        }
        publishFrame(spectrumFrameFromPcmWindow(window));
        window.clear();
    }

    void publishFrame(app::AudioVisualizationFrame frame)
    {
        std::lock_guard lock(frame_mutex_);
        if (frame_.available) {
            for (std::size_t index = 0; index < frame.bands.size(); ++index) {
                frame.bands[index] = (frame_.bands[index] * 0.58f) + (frame.bands[index] * 0.42f);
            }
        }
        frame_ = frame;
    }

    static constexpr std::size_t kWindowSize{1024};

    mutable std::mutex frame_mutex_{};
    app::AudioVisualizationFrame frame_{};
    std::thread worker_{};
    std::atomic_bool stop_requested_{false};
    RunningPipeProcess process_{};
};

class ExternalAudioPlaybackBackend final : public app::AudioPlaybackBackend {
public:
    ExternalAudioPlaybackBackend()
        : executable_(resolveAudioExecutable())
        , analyzer_executable_(resolveAnalyzerExecutable())
    {}

    ~ExternalAudioPlaybackBackend() override
    {
        stop();
    }

    [[nodiscard]] bool available() const override
    {
        return !executable_.path.empty();
    }

    [[nodiscard]] std::string displayName() const override
    {
        return executable_.windows_interop ? "WINDOWS FFPLAY" : "BUILT-IN";
    }

    bool playFile(const fs::path& path, double start_seconds) override
    {
        stop();
        if (!available() || !fs::exists(path)) {
            logRuntime(RuntimeLogLevel::Warn, "audio", "Playback unavailable for " + pathUtf8String(path));
            return false;
        }
#if defined(__linux__)
        const auto playback_input = executable_.windows_interop ? wslPathForWindowsProcess(path) : pathUtf8String(path);
        const auto analyzer_input = analyzer_executable_.windows_interop ? wslPathForWindowsProcess(path) : pathUtf8String(path);
        return playArgument(
            playback_input,
            analyzer_input,
            start_seconds,
            "Started playback: ",
            "Failed playback: ");
#else
        const auto input = pathUtf8String(path);
        return playArgument(input, input, start_seconds, "Started playback: ", "Failed playback: ");
#endif
    }

    bool playUri(const std::string& uri, double start_seconds) override
    {
        stop();
        if (!available() || uri.empty()) {
            logRuntime(RuntimeLogLevel::Warn, "audio", "Remote playback unavailable");
            return false;
        }
        return playArgument(uri, uri, start_seconds, "Started stream playback: ", "Failed stream playback: ");
    }

    bool playArgument(
        const std::string& playback_input,
        const std::string& analyzer_input,
        double start_seconds,
        const std::string& ok_prefix,
        const std::string& fail_prefix)
    {
        const bool input_is_network = networkInput(playback_input);
        std::vector<std::string> args{
            "-nodisp",
            "-autoexit",
            "-loglevel",
            "warning",
            "-nostats"};
        if (input_is_network) {
            args.emplace_back("-fflags");
            args.emplace_back("nobuffer");
            args.emplace_back("-flags");
            args.emplace_back("low_delay");
            args.emplace_back("-probesize");
            args.emplace_back("32768");
            args.emplace_back("-analyzeduration");
            args.emplace_back("0");
        }

        if (start_seconds > 0.0) {
            const auto seek_ms = static_cast<int>(std::floor(start_seconds * 1000.0));
            const int whole = seek_ms / 1000;
            const int frac = std::abs(seek_ms % 1000);
            char buffer[32]{};
            std::snprintf(buffer, sizeof(buffer), "%d.%03d", whole, frac);
            args.emplace_back("-ss");
            args.emplace_back(buffer);
        }

        args.push_back(playback_input);
        const bool ok = spawnAudioProcess(process_, executable_.path, args);
        paused_ = false;
        network_playback_ = ok && input_is_network;
        analyzer_started_ = false;
        pending_analyzer_input_ = ok ? analyzer_input : std::string{};
        pending_analyzer_start_seconds_ = start_seconds;
        playback_started_confirmed_ = ok && analyzer_executable_.path.empty() && !network_playback_;
        process_started_ = ok ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};
        if (ok && !network_playback_) {
            analyzer_.start(analyzer_executable_, analyzer_input, start_seconds);
            analyzer_started_ = !analyzer_executable_.path.empty();
        } else {
            analyzer_.stop();
        }
        logRuntime(
            ok ? RuntimeLogLevel::Info : RuntimeLogLevel::Warn,
            "audio",
            std::string(ok ? ok_prefix : fail_prefix) + playback_input + " via " + executable_.path.string());
        return ok;
    }

    void pause() override
    {
        if (!process_.active || paused_) {
            return;
        }
        pauseAudioProcess(process_);
        analyzer_.pause();
        paused_ = true;
        logRuntime(RuntimeLogLevel::Debug, "audio", "Paused playback process");
    }

    void resume() override
    {
        if (!process_.active || !paused_) {
            return;
        }
        resumeAudioProcess(process_);
        analyzer_.resume();
        paused_ = false;
        logRuntime(RuntimeLogLevel::Debug, "audio", "Resumed playback process");
    }

    void stop() override
    {
        if (process_.active) {
            logRuntime(RuntimeLogLevel::Debug, "audio", "Stopping playback process");
        }
        stopAudioProcess(process_);
        analyzer_.stop();
        paused_ = false;
        playback_started_confirmed_ = false;
        network_playback_ = false;
        analyzer_started_ = false;
        pending_analyzer_input_.clear();
        pending_analyzer_start_seconds_ = 0.0;
        process_started_ = {};
    }

    [[nodiscard]] bool isPlaying() override
    {
        return !paused_ && audioProcessRunning(process_);
    }

    [[nodiscard]] bool isFinished() override
    {
        return audioProcessFinished(process_);
    }

    [[nodiscard]] app::AudioPlaybackState state() override
    {
        if (audioProcessFinished(process_)) {
            paused_ = false;
            analyzer_.stop();
            return app::AudioPlaybackState::Finished;
        }
        if (paused_ && process_.active) {
            return app::AudioPlaybackState::Paused;
        }
        if (!audioProcessRunning(process_)) {
            return app::AudioPlaybackState::Idle;
        }
        if (!playback_started_confirmed_) {
            const auto now = std::chrono::steady_clock::now();
            const auto elapsed = process_started_ == std::chrono::steady_clock::time_point{}
                ? std::chrono::steady_clock::duration::zero()
                : now - process_started_;
            if (network_playback_) {
                if (elapsed < kNetworkPlaybackUiStartDelay) {
                    return app::AudioPlaybackState::Starting;
                }
                startAnalyzerIfNeeded();
                if (analyzer_executable_.path.empty()
                    || analyzer_.currentFrame().available
                    || elapsed >= kNetworkPlaybackUiStartDelay + kNetworkAnalyzerWarmupLimit) {
                    playback_started_confirmed_ = true;
                } else {
                    return app::AudioPlaybackState::Starting;
                }
            } else if (!analyzer_executable_.path.empty()) {
                if (analyzer_.currentFrame().available) {
                    playback_started_confirmed_ = true;
                } else if (process_started_ != std::chrono::steady_clock::time_point{}
                    && elapsed < std::chrono::seconds{45}) {
                    return app::AudioPlaybackState::Starting;
                }
            }
        }
        return app::AudioPlaybackState::Playing;
    }

    [[nodiscard]] app::AudioVisualizationFrame visualizationFrame() const override
    {
        return analyzer_.currentFrame();
    }

private:
    void startAnalyzerIfNeeded()
    {
        if (analyzer_started_ || analyzer_executable_.path.empty() || pending_analyzer_input_.empty()) {
            return;
        }
        analyzer_.start(analyzer_executable_, pending_analyzer_input_, pending_analyzer_start_seconds_);
        analyzer_started_ = true;
    }

    AudioExecutable executable_{};
    AudioExecutable analyzer_executable_{};
    RunningProcess process_{};
    RealtimePcmSpectrumAnalyzer analyzer_{};
    std::chrono::steady_clock::time_point process_started_{};
    std::string pending_analyzer_input_{};
    double pending_analyzer_start_seconds_{0.0};
    bool playback_started_confirmed_{false};
    bool network_playback_{false};
    bool analyzer_started_{false};
    bool paused_{false};
};

} // namespace

std::shared_ptr<app::AudioPlaybackBackend> createHostAudioPlaybackBackend()
{
    return std::make_shared<ExternalAudioPlaybackBackend>();
}

} // namespace lofibox::platform::host
