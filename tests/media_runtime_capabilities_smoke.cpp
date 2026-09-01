// SPDX-License-Identifier: GPL-3.0-or-later

#include <algorithm>
#include <cassert>
#include <filesystem>

#include "platform/host/media_runtime_capabilities.h"

int main()
{
    using lofibox::platform::host::MediaRuntimeCapabilities;
    using lofibox::platform::host::missingWidgetMediaRuntimeTools;

    const auto missing_everything = missingWidgetMediaRuntimeTools({});
    assert(missing_everything.size() == 3U);
    assert(std::find(missing_everything.begin(), missing_everything.end(), "ffmpeg") != missing_everything.end());
    assert(std::find(missing_everything.begin(), missing_everything.end(), "ffprobe") != missing_everything.end());
    assert(std::find(missing_everything.begin(), missing_everything.end(), "paplay or aplay") != missing_everything.end());

    MediaRuntimeCapabilities complete{};
    complete.ffmpeg = std::filesystem::path{"/usr/bin/ffmpeg"};
    complete.ffprobe = std::filesystem::path{"/usr/bin/ffprobe"};
    complete.paplay = std::filesystem::path{"/usr/bin/paplay"};
    assert(missingWidgetMediaRuntimeTools(complete).empty());

    complete.paplay.reset();
    complete.aplay = std::filesystem::path{"/usr/bin/aplay"};
    assert(missingWidgetMediaRuntimeTools(complete).empty());
    return 0;
}
