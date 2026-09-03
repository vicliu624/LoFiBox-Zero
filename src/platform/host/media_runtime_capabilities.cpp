// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/media_runtime_capabilities.h"

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

} // namespace

const MediaRuntimeCapabilities& mediaRuntimeCapabilities()
{
    static const MediaRuntimeCapabilities capabilities = discoverMediaRuntimeCapabilities();
    return capabilities;
}

} // namespace lofibox::platform::host
