// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/runtime_enrichment_clients.h"
#include "platform/host/runtime_enrichment_client_helpers.h"
#include "platform/host/runtime_logger.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace lofibox::platform::host::runtime_detail {
FfprobeMetadataReader::FfprobeMetadataReader()
{
#if defined(_WIN32)
    executable_path_ = resolveExecutablePath(L"FFPROBE_PATH", L"ffprobe.exe");
#elif defined(__linux__)
    executable_path_ = resolveExecutablePath("FFPROBE_PATH", "ffprobe");
#endif
}

bool FfprobeMetadataReader::available() const
{
    return executable_path_.has_value();
}

app::TrackMetadata FfprobeMetadataReader::read(const fs::path& path) const
{
    if (!executable_path_ || !fs::exists(path)) {
        return {};
    }

    const std::vector<std::string> args{
        "-v",
        "error",
        "-select_streams",
        "a:0",
        "-show_entries",
        "format=duration:format_tags=title,artist,album,genre,composer:stream_tags=title,artist,album,genre,composer",
        "-of",
        "default=noprint_wrappers=1:nokey=0",
        pathUtf8String(path)};

    if (const auto output = captureProcessOutput(*executable_path_, args)) {
        return parseMetadataOutput(*output);
    }
    return {};
}

} // namespace lofibox::platform::host::runtime_detail

