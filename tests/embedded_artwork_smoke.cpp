// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <iostream>

#include "media_fixture_utils.h"
#include "platform/host/runtime_services_factory.h"

namespace fs = std::filesystem;

int main()
{
    #if defined(_WIN32)
    const auto ffmpeg_path = test_media_fixture::resolveExecutablePath(L"FFMPEG_PATH", L"ffmpeg.exe");
    #elif defined(__linux__)
    const auto ffmpeg_path = test_media_fixture::resolveExecutablePath("FFMPEG_PATH", "ffmpeg");
    #else
    const auto ffmpeg_path = std::optional<fs::path>{};
    #endif
    if (!ffmpeg_path.has_value()) {
        std::cout << "ffmpeg not found; skipping artwork smoke.\n";
        return 0;
    }

    const fs::path root = fs::temp_directory_path() / "lofibox_zero_embedded_artwork_smoke";
    std::error_code ec{};
    fs::remove_all(root, ec);
    fs::create_directories(root, ec);
    if (ec) {
        std::cerr << "Failed to create temp directory.\n";
        return 1;
    }

    const fs::path cover_path = root / "cover.png";
    const std::vector<std::string> cover_args{
        "-loglevel", "error",
        "-nostdin",
        "-f", "lavfi",
        "-i", "color=c=red:s=32x32:d=1",
        "-frames:v", "1",
        "-y",
        cover_path.string()};
    if (!test_media_fixture::runCommand(*ffmpeg_path, cover_args) || !fs::exists(cover_path)) {
        std::cerr << "Failed to generate cover fixture.\n";
        fs::remove_all(root, ec);
        return 1;
    }

    const fs::path audio_path = root / "artwork.mp3";
    const std::vector<std::string> audio_args{
        "-loglevel", "error",
        "-nostdin",
        "-f", "lavfi",
        "-i", "sine=frequency=1000:duration=1",
        "-i", cover_path.string(),
        "-map", "0:a",
        "-map", "1:v",
        "-c:a", "libmp3lame",
        "-c:v", "mjpeg",
        "-id3v2_version", "3",
        "-metadata:s:v", "title=Album cover",
        "-metadata:s:v", "comment=Cover (front)",
        "-y",
        audio_path.string()};
    if (!test_media_fixture::runCommand(*ffmpeg_path, audio_args) || !fs::exists(audio_path)) {
        std::cerr << "Failed to generate artwork-tagged audio fixture.\n";
        fs::remove_all(root, ec);
        return 1;
    }

    auto services = lofibox::platform::host::createHostRuntimeServices();
    if (!services.artwork_provider->available()) {
        std::cout << "artwork provider unavailable; skipping artwork smoke.\n";
        fs::remove_all(root, ec);
        return 0;
    }

    const auto artwork = services.artwork_provider->read(audio_path);
    if (!artwork.has_value()) {
        std::cerr << "Expected embedded artwork to be extracted.\n";
        fs::remove_all(root, ec);
        return 1;
    }
    if (artwork->width() <= 0 || artwork->height() <= 0) {
        std::cerr << "Expected extracted artwork canvas to have valid size.\n";
        fs::remove_all(root, ec);
        return 1;
    }

    fs::remove_all(root, ec);
    return 0;
}
