// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/runtime_provider_factories.h"

#include <cmath>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "platform/host/png_canvas_loader.h"
#include "platform/host/runtime_logger.h"

namespace fs = std::filesystem;

namespace lofibox::platform::host {
namespace {
using namespace runtime_detail;
class ExternalAudioPlaybackBackend final : public app::AudioPlaybackBackend {
public:
    ExternalAudioPlaybackBackend()
    {
#if defined(_WIN32)
        executable_path_ = resolveExecutablePath(L"FFPLAY_PATH", L"ffplay.exe");
#elif defined(__linux__)
        executable_path_ = resolveExecutablePath("FFPLAY_PATH", "ffplay");
#endif
    }

    ~ExternalAudioPlaybackBackend() override
    {
        stop();
    }

    [[nodiscard]] bool available() const override
    {
        return executable_path_.has_value();
    }

    [[nodiscard]] std::string displayName() const override
    {
        return "BUILT-IN";
    }

    bool playFile(const fs::path& path, double start_seconds) override
    {
        stop();
        if (!executable_path_ || !fs::exists(path)) {
            logRuntime(RuntimeLogLevel::Warn, "audio", "Playback unavailable for " + pathUtf8String(path));
            return false;
        }
        return playArgument(pathUtf8String(path), start_seconds, "Started playback: ", "Failed playback: ");
    }

    bool playUri(const std::string& uri, double start_seconds) override
    {
        stop();
        if (!executable_path_ || uri.empty()) {
            logRuntime(RuntimeLogLevel::Warn, "audio", "Remote playback unavailable");
            return false;
        }
        return playArgument(uri, start_seconds, "Started stream playback: ", "Failed stream playback: ");
    }

    bool playArgument(const std::string& input, double start_seconds, const std::string& ok_prefix, const std::string& fail_prefix)
    {
        std::vector<std::string> args{
            "-nodisp",
            "-autoexit",
            "-loglevel",
            "quiet",
            "-nostats"};

        if (start_seconds > 0.0) {
            const auto seek_ms = static_cast<int>(std::floor(start_seconds * 1000.0));
            const int whole = seek_ms / 1000;
            const int frac = std::abs(seek_ms % 1000);
            char buffer[32]{};
            std::snprintf(buffer, sizeof(buffer), "%d.%03d", whole, frac);
            args.emplace_back("-ss");
            args.emplace_back(buffer);
        }

        args.push_back(input);
        const bool ok = spawnAudioProcess(process_, *executable_path_, args);
        logRuntime(ok ? RuntimeLogLevel::Info : RuntimeLogLevel::Warn, "audio", std::string(ok ? ok_prefix : fail_prefix) + input);
        return ok;
    }

    void stop() override
    {
        if (process_.active) {
            logRuntime(RuntimeLogLevel::Debug, "audio", "Stopping playback process");
        }
        stopAudioProcess(process_);
    }

    [[nodiscard]] bool isPlaying() override
    {
        return audioProcessRunning(process_);
    }

    [[nodiscard]] bool isFinished() override
    {
        return audioProcessFinished(process_);
    }

private:
    std::optional<fs::path> executable_path_{};
    RunningProcess process_{};
};

} // namespace

std::shared_ptr<app::AudioPlaybackBackend> createHostAudioPlaybackBackend()
{
    return std::make_shared<ExternalAudioPlaybackBackend>();
}

} // namespace lofibox::platform::host
