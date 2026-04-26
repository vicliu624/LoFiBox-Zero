// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/runtime_enrichment_clients.h"
#include "platform/host/runtime_enrichment_client_helpers.h"
#include "platform/host/runtime_logger.h"
#include "platform/host/png_canvas_loader.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace lofibox::platform::host::runtime_detail {
FfmpegArtworkExtractor::FfmpegArtworkExtractor()
{
#if defined(_WIN32)
    executable_path_ = resolveExecutablePath(L"FFMPEG_PATH", L"ffmpeg.exe");
#elif defined(__linux__)
    executable_path_ = resolveExecutablePath("FFMPEG_PATH", "ffmpeg");
#endif
}

bool FfmpegArtworkExtractor::available() const
{
    return executable_path_.has_value();
}

std::optional<core::Canvas> FfmpegArtworkExtractor::extractEmbedded(const fs::path& path, const fs::path& cache_path) const
{
    if (!executable_path_ || !fs::exists(path)) {
        return std::nullopt;
    }
    std::error_code ec{};
    fs::create_directories(cache_path.parent_path(), ec);
    fs::remove(cache_path, ec);
    const std::vector<std::string> args{
        "-loglevel", "error",
        "-nostdin",
        "-i", pathUtf8String(path),
        "-map", "0:v:0?",
        "-frames:v", "1",
        "-c:v", "png",
        "-y", pathUtf8String(cache_path)};

    if (!runProcess(*executable_path_, args) || !fs::exists(cache_path, ec) || ec) {
        fs::remove(cache_path, ec);
        return std::nullopt;
    }

    return loadPngCanvas(cache_path);
}

std::optional<core::Canvas> FfmpegArtworkExtractor::materialize(const fs::path& candidate, const fs::path& cache_path) const
{
    if (!executable_path_) {
        return std::nullopt;
    }
    std::error_code ec{};
    fs::create_directories(cache_path.parent_path(), ec);
    fs::remove(cache_path, ec);
    const std::vector<std::string> args{
        "-loglevel", "error",
        "-nostdin",
        "-i", pathUtf8String(candidate),
        "-frames:v", "1",
        "-y", pathUtf8String(cache_path)};
    if (!runProcess(*executable_path_, args) || !fs::exists(cache_path, ec) || ec) {
        fs::remove(cache_path, ec);
        return std::nullopt;
    }
    return loadPngCanvas(cache_path);
}

CoverArtArchiveClient::CoverArtArchiveClient()
{
#if defined(_WIN32)
    curl_path_ = resolveExecutablePath(L"CURL_PATH", L"curl.exe");
#elif defined(__linux__)
    curl_path_ = resolveExecutablePath("CURL_PATH", "curl");
#endif
}

bool CoverArtArchiveClient::available() const
{
    return extractor_.available() && (curl_path_.has_value() || resolvePythonPath().has_value());
}

bool CoverArtArchiveClient::fetchFrontCover(std::string_view release_mbid, const fs::path& output_path) const
{
    if (!available() || release_mbid.empty()) {
        return false;
    }
    const std::string url = "https://coverartarchive.org/release/" + std::string(release_mbid) + "/front-250";
    return downloadUrlToPngFile(curl_path_, extractor_, url, output_path);
}

bool CoverArtArchiveClient::fetchReleaseGroupFrontCover(std::string_view release_group_mbid, const fs::path& output_path) const
{
    if (!available() || release_group_mbid.empty()) {
        return false;
    }
    const std::string url = "https://coverartarchive.org/release-group/" + std::string(release_group_mbid) + "/front-250";
    return downloadUrlToPngFile(curl_path_, extractor_, url, output_path);
}

} // namespace lofibox::platform::host::runtime_detail

