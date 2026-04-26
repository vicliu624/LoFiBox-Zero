// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <iostream>
#include <optional>
#include <string>

#include "platform/host/runtime_services_factory.h"
#include "media_fixture_utils.h"

namespace fs = std::filesystem;

int main()
{
#if defined(_WIN32)
    const auto ffmpeg_path = test_media_fixture::resolveExecutablePath(L"FFMPEG_PATH", L"ffmpeg.exe");
    const wchar_t* original_root_env = _wgetenv(L"LOFIBOX_MEDIA_ROOT");
    const auto original_root = original_root_env != nullptr ? std::optional<std::wstring>(original_root_env) : std::nullopt;
#elif defined(__linux__)
    const auto ffmpeg_path = test_media_fixture::resolveExecutablePath("FFMPEG_PATH", "ffmpeg");
    const char* original_root = std::getenv("LOFIBOX_MEDIA_ROOT");
#else
    const auto ffmpeg_path = std::optional<fs::path>{};
#endif
    if (!ffmpeg_path.has_value()) {
        std::cout << "ffmpeg not found; skipping root directory artwork guard smoke.\n";
        return 0;
    }

    const fs::path root = fs::temp_directory_path() / "lofibox_zero_root_directory_artwork_guard";
    std::error_code ec{};
    fs::remove_all(root, ec);
    fs::create_directories(root, ec);
    if (ec) {
        std::cerr << "Failed to create temp directory.\n";
        return 1;
    }

    const fs::path cover_png = root / "generic.png";
    const fs::path folder_jpg = root / "Folder.jpg";
    const std::vector<std::string> png_args{
        "-loglevel", "error",
        "-nostdin",
        "-f", "lavfi",
        "-i", "color=c=blue:s=40x40:d=1",
        "-frames:v", "1",
        "-y",
        cover_png.string()};
    if (!test_media_fixture::runCommand(*ffmpeg_path, png_args) || !fs::exists(cover_png)) {
        std::cerr << "Failed to generate PNG cover fixture.\n";
        fs::remove_all(root, ec);
        return 1;
    }

    const std::vector<std::string> jpg_args{
        "-loglevel", "error",
        "-nostdin",
        "-i", cover_png.string(),
        "-frames:v", "1",
        "-y",
        folder_jpg.string()};
    if (!test_media_fixture::runCommand(*ffmpeg_path, jpg_args) || !fs::exists(folder_jpg)) {
        std::cerr << "Failed to generate Folder.jpg fixture.\n";
        fs::remove_all(root, ec);
        return 1;
    }

    const fs::path audio_path = root / "root-track.mp3";
    const std::vector<std::string> audio_args{
        "-loglevel", "error",
        "-nostdin",
        "-f", "lavfi",
        "-i", "sine=frequency=880:duration=1",
        "-i", cover_png.string(),
        "-map", "0:a",
        "-map", "1:v",
        "-c:a", "libmp3lame",
        "-c:v", "mjpeg",
        "-id3v2_version", "3",
        "-metadata", "title=Root Song",
        "-metadata", "artist=Root Artist",
        "-metadata", "album=Root Album",
        "-metadata:s:v", "title=Album cover",
        "-metadata:s:v", "comment=Cover (front)",
        "-y",
        audio_path.string()};
    if (!test_media_fixture::runCommand(*ffmpeg_path, audio_args) || !fs::exists(audio_path)) {
        std::cerr << "Failed to generate contaminated MP3 fixture.\n";
        fs::remove_all(root, ec);
        return 1;
    }

#if defined(_WIN32)
    _wputenv_s(L"LOFIBOX_MEDIA_ROOT", root.wstring().c_str());
    SetEnvironmentVariableW(L"LOFIBOX_MEDIA_ROOT", root.wstring().c_str());
#elif defined(__linux__)
    setenv("LOFIBOX_MEDIA_ROOT", root.string().c_str(), 1);
#endif

    auto services = lofibox::platform::host::createHostRuntimeServices();
    const auto artwork = services.artwork_provider->read(audio_path);

#if defined(_WIN32)
    if (original_root.has_value()) {
        _wputenv_s(L"LOFIBOX_MEDIA_ROOT", original_root->c_str());
        SetEnvironmentVariableW(L"LOFIBOX_MEDIA_ROOT", original_root->c_str());
    } else {
        _wputenv_s(L"LOFIBOX_MEDIA_ROOT", L"");
        SetEnvironmentVariableW(L"LOFIBOX_MEDIA_ROOT", nullptr);
    }
#elif defined(__linux__)
    if (original_root != nullptr) {
        setenv("LOFIBOX_MEDIA_ROOT", original_root, 1);
    } else {
        unsetenv("LOFIBOX_MEDIA_ROOT");
    }
#endif

    if (artwork.has_value()) {
        std::cerr << "Expected provider to ignore root-level generic artwork contamination.\n";
        fs::remove_all(root, ec);
        return 1;
    }

    fs::remove_all(root, ec);
    return 0;
}
