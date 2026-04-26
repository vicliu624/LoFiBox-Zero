// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <initializer_list>
#include <iostream>
#include <string>
#include <vector>

#include "media_fixture_utils.h"
#include "platform/host/runtime_host_internal.h"
#include "platform/host/runtime_services_factory.h"

namespace fs = std::filesystem;

namespace {

std::string bytes(std::initializer_list<unsigned char> values)
{
    std::string result{};
    result.reserve(values.size());
    for (const auto value : values) {
        result.push_back(static_cast<char>(value));
    }
    return result;
}

} // namespace

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
        std::cout << "ffmpeg not found; skipping metadata generation smoke.\n";
        return 0;
    }

    const fs::path root = fs::temp_directory_path() / "lofibox_zero_embedded_metadata_smoke";
    std::error_code ec{};
    fs::remove_all(root, ec);
    fs::create_directories(root, ec);
    if (ec) {
        std::cerr << "Failed to create temp directory.\n";
        return 1;
    }

    const fs::path audio_path = root / "tagged.flac";
    const std::vector<std::string> args{
        "-loglevel", "error",
        "-nostdin",
        "-f", "lavfi",
        "-i", "sine=frequency=1000:duration=1",
        "-c:a", "flac",
        "-metadata", "Title=Test Title",
        "-metadata", "Artist=Test Artist",
        "-metadata", "Album=Test Album",
        "-metadata", "Genre=Test Genre",
        "-metadata", "Composer=Test Composer",
        "-y",
        audio_path.string()};

    if (!test_media_fixture::runCommand(*ffmpeg_path, args) || !fs::exists(audio_path)) {
        std::cerr << "Failed to generate tagged audio fixture.\n";
        fs::remove_all(root, ec);
        return 1;
    }

    auto services = lofibox::platform::host::createHostRuntimeServices();
    if (!services.metadata.metadata_provider->available()) {
        std::cout << "metadata provider unavailable; skipping metadata smoke.\n";
        fs::remove_all(root, ec);
        return 0;
    }

    const auto metadata = services.metadata.metadata_provider->read(audio_path);
    if (!metadata.title || *metadata.title != "Test Title") {
        std::cerr << "Expected title metadata.\n";
        fs::remove_all(root, ec);
        return 1;
    }
    if (!metadata.artist || *metadata.artist != "Test Artist") {
        std::cerr << "Expected artist metadata.\n";
        fs::remove_all(root, ec);
        return 1;
    }
    if (!metadata.album || *metadata.album != "Test Album") {
        std::cerr << "Expected album metadata.\n";
        fs::remove_all(root, ec);
        return 1;
    }
    if (!metadata.genre || *metadata.genre != "Test Genre") {
        std::cerr << "Expected genre metadata.\n";
        fs::remove_all(root, ec);
        return 1;
    }
    if (!metadata.composer || *metadata.composer != "Test Composer") {
        std::cerr << "Expected composer metadata.\n";
        fs::remove_all(root, ec);
        return 1;
    }
    if (!metadata.duration_seconds || *metadata.duration_seconds <= 0) {
        std::cerr << "Expected positive duration metadata.\n";
        fs::remove_all(root, ec);
        return 1;
    }

    const auto repeated_stream_metadata = lofibox::platform::host::runtime_detail::parseMetadataOutput(
        "TAG:title=Stream Title\n"
        "TAG:artist=Stream Artist\n"
        "TAG:album=Stream Album\n"
        "TAG:title=Cover\n");
    if (!repeated_stream_metadata.title || *repeated_stream_metadata.title != "Stream Title") {
        std::cerr << "Expected first stream title to survive cover-stream metadata.\n";
        fs::remove_all(root, ec);
        return 1;
    }
    if (!repeated_stream_metadata.artist || *repeated_stream_metadata.artist != "Stream Artist") {
        std::cerr << "Expected stream artist metadata.\n";
        fs::remove_all(root, ec);
        return 1;
    }

    const auto escaped_title = lofibox::platform::host::runtime_detail::extractJsonString(
        "{\"title\":\"\\u8bf4\\u597d\\u4e0d\\u54ed\",\"artist\":\"\\u5468\\u6770\\u4f26\"}",
        "\"title\":\"");
    if (!escaped_title || *escaped_title != "说好不哭") {
        std::cerr << "Expected JSON unicode escapes to decode to UTF-8 metadata.\n";
        fs::remove_all(root, ec);
        return 1;
    }

    const auto escaped_field = lofibox::platform::host::runtime_detail::repairMetadataText("u8bf4u597du4e0du54ed");
    if (escaped_field != bytes({0xE8, 0xAF, 0xB4, 0xE5, 0xA5, 0xBD, 0xE4, 0xB8, 0x8D, 0xE5, 0x93, 0xAD})) {
        std::cerr << "Expected bare unicode-escape damaged metadata to be repaired.\n";
        fs::remove_all(root, ec);
        return 1;
    }

    const auto mojibake_metadata = lofibox::platform::host::runtime_detail::parseMetadataOutput(
        bytes({
            0x54, 0x41, 0x47, 0x3A, 0x74, 0x69, 0x74, 0x6C, 0x65, 0x3D, 0xC3, 0xA8, 0xC2, 0xAF, 0xC2, 0xB4, 0xC3, 0xA5, 0xC2, 0xA5, 0xC2, 0xBD, 0xC3, 0xA4, 0xC2, 0xB8, 0xC2, 0x8D, 0xC3, 0xA5, 0xC2, 0x93, 0xC2, 0xAD, 0x20, 0x28, 0x77, 0x69, 0x74, 0x68, 0x20, 0xC3, 0xA4, 0xC2, 0xBA, 0xC2, 0x94, 0xC3, 0xA6, 0xC2, 0x9C, 0xC2, 0x88, 0xC3, 0xA5, 0xC2, 0xA4, 0xC2, 0xA9, 0xC3, 0xA9, 0xC2, 0x98, 0xC2, 0xBF, 0xC3, 0xA4, 0xC2, 0xBF, 0xC2, 0xA1, 0x29, 0x0A, 0x54, 0x41, 0x47, 0x3A, 0x61, 0x72, 0x74, 0x69, 0x73, 0x74, 0x3D, 0xC3, 0xA5, 0xC2, 0x91, 0xC2, 0xA8, 0xC3, 0xA6, 0xC2, 0x9D, 0xC2, 0xB0, 0xC3, 0xA4, 0xC2, 0xBC, 0xC2, 0xA6, 0x0A, 0x54, 0x41, 0x47, 0x3A, 0x61, 0x6C, 0x62, 0x75, 0x6D, 0x3D, 0xC3, 0xA8, 0xC2, 0xAF, 0xC2, 0xB4, 0xC3, 0xA5, 0xC2, 0xA5, 0xC2, 0xBD, 0xC3, 0xA4, 0xC2, 0xB8, 0xC2, 0x8D, 0xC3, 0xA5, 0xC2, 0x93, 0xC2, 0xAD, 0xC3, 0xAF, 0xC2, 0xBC, 0xC2, 0x88, 0x77, 0x69, 0x74, 0x68, 0x20, 0xC3, 0xA4, 0xC2, 0xBA, 0xC2, 0x94, 0xC3, 0xA6, 0xC2, 0x9C, 0xC2, 0x88, 0xC3, 0xA5, 0xC2, 0xA4, 0xC2, 0xA9, 0xC3, 0xA9, 0xC2, 0x98, 0xC2, 0xBF, 0xC3, 0xA4, 0xC2, 0xBF, 0xC2, 0xA1, 0xC3, 0xAF, 0xC2, 0xBC, 0xC2, 0x89, 0x0A}));
    if (!mojibake_metadata.title
        || *mojibake_metadata.title != bytes({0xE8, 0xAF, 0xB4, 0xE5, 0xA5, 0xBD, 0xE4, 0xB8, 0x8D, 0xE5, 0x93, 0xAD, 0x20, 0x28, 0x77, 0x69, 0x74, 0x68, 0x20, 0xE4, 0xBA, 0x94, 0xE6, 0x9C, 0x88, 0xE5, 0xA4, 0xA9, 0xE9, 0x98, 0xBF, 0xE4, 0xBF, 0xA1, 0x29})) {
        std::cerr << "Expected Latin-1 mojibake title metadata to be repaired.\n";
        fs::remove_all(root, ec);
        return 1;
    }
    if (!mojibake_metadata.artist
        || *mojibake_metadata.artist != bytes({0xE5, 0x91, 0xA8, 0xE6, 0x9D, 0xB0, 0xE4, 0xBC, 0xA6})) {
        std::cerr << "Expected Latin-1 mojibake artist metadata to be repaired.\n";
        fs::remove_all(root, ec);
        return 1;
    }

    const fs::path ogg_path = root / "tagged.ogg";
    const std::vector<std::string> ogg_args{
        "-loglevel", "error",
        "-nostdin",
        "-f", "lavfi",
        "-i", "sine=frequency=880:duration=1",
        "-c:a", "libvorbis",
        "-metadata", "Title=Ogg Title",
        "-metadata", "Artist=Ogg Artist",
        "-metadata", "Album=Ogg Album",
        "-y",
        ogg_path.string()};
    if (!test_media_fixture::runCommand(*ffmpeg_path, ogg_args) || !fs::exists(ogg_path)) {
        std::cout << "Failed to generate Ogg fixture; skipping Ogg metadata assertion.\n";
    } else {
        const auto ogg_metadata = services.metadata.metadata_provider->read(ogg_path);
        if (!ogg_metadata.title || *ogg_metadata.title != "Ogg Title") {
            std::cerr << "Expected Ogg stream title metadata.\n";
            fs::remove_all(root, ec);
            return 1;
        }
        if (!ogg_metadata.artist || *ogg_metadata.artist != "Ogg Artist") {
            std::cerr << "Expected Ogg stream artist metadata.\n";
            fs::remove_all(root, ec);
            return 1;
        }
        if (!ogg_metadata.album || *ogg_metadata.album != "Ogg Album") {
            std::cerr << "Expected Ogg stream album metadata.\n";
            fs::remove_all(root, ec);
            return 1;
        }
    }

    fs::remove_all(root, ec);
    return 0;
}
