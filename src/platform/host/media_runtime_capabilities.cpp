// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/media_runtime_capabilities.h"

#include <cstddef>
#include <stdexcept>
#include <string>

#include "platform/host/runtime_host_internal.h"

namespace lofibox::platform::host {
namespace {

MediaRuntimeCapabilities discoverMediaRuntimeCapabilities()
{
    MediaRuntimeCapabilities capabilities{};
#if defined(_WIN32)
    capabilities.ffmpeg = runtime_detail::resolveExecutablePath(L"FFMPEG_PATH", L"ffmpeg.exe");
    capabilities.ffprobe = runtime_detail::resolveExecutablePath(L"FFPROBE_PATH", L"ffprobe.exe");
#elif defined(__linux__)
    capabilities.ffmpeg = runtime_detail::resolveExecutablePath("FFMPEG_PATH", "ffmpeg");
    capabilities.ffprobe = runtime_detail::resolveExecutablePath("FFPROBE_PATH", "ffprobe");
    capabilities.paplay = runtime_detail::resolveExecutablePath("PAPLAY_PATH", "paplay");
    capabilities.aplay = runtime_detail::resolveExecutablePath("APLAY_PATH", "aplay");
#endif
    return capabilities;
}

std::string joinMissingTools(const std::vector<std::string>& missing)
{
    std::string message{};
    for (std::size_t index = 0; index < missing.size(); ++index) {
        if (index != 0U) {
            message += ", ";
        }
        message += missing[index];
    }
    return message;
}

} // namespace

const MediaRuntimeCapabilities& mediaRuntimeCapabilities()
{
    static const MediaRuntimeCapabilities capabilities = discoverMediaRuntimeCapabilities();
    return capabilities;
}

std::vector<std::string> missingWidgetMediaRuntimeTools(const MediaRuntimeCapabilities& capabilities)
{
    std::vector<std::string> missing{};
    if (!capabilities.ffmpeg) {
        missing.emplace_back("ffmpeg");
    }
    if (!capabilities.ffprobe) {
        missing.emplace_back("ffprobe");
    }
    if (!capabilities.paplay && !capabilities.aplay) {
        missing.emplace_back("paplay or aplay");
    }
    return missing;
}

void requireWidgetMediaRuntime()
{
    const auto missing = missingWidgetMediaRuntimeTools(mediaRuntimeCapabilities());
    if (missing.empty()) {
        return;
    }

    throw std::runtime_error(
        "TDVP media runtime is incomplete: missing " + joinMissingTools(missing)
        + ". Install the ABI-matched TDVP media runtime through the configured opkg feed, then relaunch LoFiBox.");
}

} // namespace lofibox::platform::host
