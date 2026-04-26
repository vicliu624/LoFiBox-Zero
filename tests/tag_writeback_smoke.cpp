// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>

#include "app/runtime_services.h"
#include "platform/host/runtime_provider_factories.h"
#include "platform/host/runtime_services_factory.h"
#include "platform/host/runtime_host_internal.h"
#include "media_fixture_utils.h"

namespace fs = std::filesystem;

namespace {
class OfflineConnectivityProvider final : public lofibox::app::ConnectivityProvider {
public:
    [[nodiscard]] std::string displayName() const override { return "offline test"; }
    [[nodiscard]] bool connected() const override { return false; }
};
}

int main()
{
#if defined(_WIN32)
    const auto ffmpeg_path = test_media_fixture::resolveExecutablePath(L"FFMPEG_PATH", L"ffmpeg.exe");
    const auto python_path = test_media_fixture::resolveExecutablePath(L"PYTHON_EXECUTABLE", L"python.exe");
#elif defined(__linux__)
    const auto ffmpeg_path = test_media_fixture::resolveExecutablePath("FFMPEG_PATH", "ffmpeg");
    const auto python_path = test_media_fixture::resolveExecutablePath("PYTHON_EXECUTABLE", "python3");
#else
    const auto ffmpeg_path = std::optional<fs::path>{};
    const auto python_path = std::optional<fs::path>{};
#endif

    if (!ffmpeg_path.has_value() || !python_path.has_value()) {
        std::cout << "ffmpeg/python unavailable; skipping tag writeback smoke.\n";
        return 0;
    }

    const fs::path root = fs::temp_directory_path() / "lofibox_zero_tag_writeback_smoke";
    std::error_code ec{};
    fs::remove_all(root, ec);
    fs::create_directories(root, ec);
    if (ec) {
        std::cerr << "Failed to create temp directory.\n";
        return 1;
    }

    const fs::path cover_path = root / "cover.png";
    {
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
    }

    const fs::path audio_path = root / "writeback.mp3";
    {
        const std::vector<std::string> audio_args{
            "-loglevel", "error",
            "-nostdin",
            "-f", "lavfi",
            "-i", "sine=frequency=1000:duration=1",
            "-c:a", "libmp3lame",
            "-y",
            audio_path.string()};
        if (!test_media_fixture::runCommand(*ffmpeg_path, audio_args) || !fs::exists(audio_path)) {
            std::cerr << "Failed to generate audio fixture.\n";
            fs::remove_all(root, ec);
            return 1;
        }
    }

    auto services = lofibox::platform::host::createHostRuntimeServices();
    if (!services.tag_writer->available()) {
        std::cout << "tag writer unavailable; skipping writeback smoke.\n";
        fs::remove_all(root, ec);
        return 0;
    }

    lofibox::app::TagWriteRequest request{};
    request.only_fill_missing = false;
    request.metadata = lofibox::app::TrackMetadata{};
    request.metadata->title = "Writeback Title";
    request.metadata->artist = "Writeback Artist";
    request.metadata->album = "Writeback Album";
    request.metadata->genre = "Writeback Genre";
    request.metadata->composer = "Writeback Composer";
    request.identity = lofibox::app::TrackIdentity{};
    request.identity->recording_mbid = "recording-test-mbid";
    request.identity->release_mbid = "release-test-mbid";
    request.identity->release_group_mbid = "release-group-test-mbid";
    request.identity->source = "ACOUSTID";
    request.identity->confidence = 0.98;
    request.identity->audio_fingerprint_verified = true;
    request.artwork_png_path = cover_path;
    request.lyrics = lofibox::app::TrackLyrics{};
    request.lyrics->plain = "Writeback plain lyrics";
    request.lyrics->synced = "[00:01.00]Writeback first line\n[00:02.00]Writeback second line";
    request.lyrics->source = "TEST";

    if (!services.tag_writer->write(audio_path, request)) {
        std::cerr << "Expected tag writer to write metadata back to file.\n";
        fs::remove_all(root, ec);
        return 1;
    }

    const auto metadata = services.metadata_provider->read(audio_path);
    if (!metadata.title || *metadata.title != "Writeback Title") {
        std::cerr << "Expected written title metadata.\n";
        fs::remove_all(root, ec);
        return 1;
    }
    if (!metadata.artist || *metadata.artist != "Writeback Artist") {
        std::cerr << "Expected written artist metadata.\n";
        fs::remove_all(root, ec);
        return 1;
    }
    if (!metadata.album || *metadata.album != "Writeback Album") {
        std::cerr << "Expected written album metadata.\n";
        fs::remove_all(root, ec);
        return 1;
    }

    const auto artwork = services.artwork_provider->read(audio_path);
    if (!artwork.has_value()) {
        std::cerr << "Expected written artwork to be readable.\n";
        fs::remove_all(root, ec);
        return 1;
    }

    const fs::path python_check = root / "check_lyrics.py";
    {
        std::ofstream script(python_check, std::ios::binary | std::ios::trunc);
        script
            << "import sys\n"
            << "import mutagen\n"
            << "from mutagen.id3 import ID3\n"
            << "audio = mutagen.File(sys.argv[1], easy=False)\n"
            << "tags = ID3(sys.argv[1])\n"
            << "has_uslt = bool(tags.getall('USLT'))\n"
            << "has_recording = any('recording-test-mbid' in str(getattr(frame, 'text', '')) for frame in tags.getall('TXXX:MusicBrainz Track Id'))\n"
            << "has_source = any('ACOUSTID' in str(getattr(frame, 'text', '')) for frame in tags.getall('TXXX:LoFiBox Identity Source'))\n"
            << "sys.exit(0 if has_uslt and has_recording and has_source else 1)\n";
    }

    const std::vector<std::string> python_args{
        python_check.string(),
        audio_path.string()};
    if (!test_media_fixture::runCommand(*python_path, python_args)) {
        std::cerr << "Expected written lyrics and identity to be present in file tags.\n";
        fs::remove_all(root, ec);
        return 1;
    }

    const auto readback_lyrics = services.lyrics_provider->fetch(audio_path, metadata);
    if (!readback_lyrics.plain || readback_lyrics.plain->find("Writeback plain lyrics") == std::string::npos) {
        std::cerr << "Expected lyrics provider to read embedded lyrics back from file tags.\n";
        fs::remove_all(root, ec);
        return 1;
    }
    if (!readback_lyrics.synced || readback_lyrics.synced->find("\n[00:02.00]") == std::string::npos) {
        std::cerr << "Expected synced lyrics readback to preserve LRC line breaks.\n";
        fs::remove_all(root, ec);
        return 1;
    }

    const fs::path collapsed_audio_path = root / "collapsed.mp3";
    {
        const std::vector<std::string> audio_args{
            "-loglevel", "error",
            "-nostdin",
            "-f", "lavfi",
            "-i", "sine=frequency=880:duration=1",
            "-c:a", "libmp3lame",
            "-y",
            collapsed_audio_path.string()};
        if (!test_media_fixture::runCommand(*ffmpeg_path, audio_args) || !fs::exists(collapsed_audio_path)) {
            std::cerr << "Failed to generate collapsed lyrics fixture.\n";
            fs::remove_all(root, ec);
            return 1;
        }
    }

    const fs::path collapsed_script = root / "write_collapsed_lyrics.py";
    {
        std::ofstream script(collapsed_script, std::ios::binary | std::ios::trunc);
        script
            << "import sys\n"
            << "from mutagen.id3 import ID3, TXXX\n"
            << "tags = ID3(sys.argv[1])\n"
            << "tags.add(TXXX(encoding=3, desc='SYNCEDLYRICS', text=['[00:01.00]First linen[00:02.00]Second line']))\n"
            << "tags.save(sys.argv[1], v2_version=3)\n";
    }
    if (!test_media_fixture::runCommand(*python_path, {collapsed_script.string(), collapsed_audio_path.string()})) {
        std::cerr << "Failed to write collapsed lyrics fixture.\n";
        fs::remove_all(root, ec);
        return 1;
    }

    const auto repaired_lyrics = services.lyrics_provider->fetch(collapsed_audio_path, {});
    if (!repaired_lyrics.synced || repaired_lyrics.synced->find("line\n[00:02.00]") == std::string::npos) {
        std::cerr << "Expected embedded lyrics reader to repair collapsed LRC line breaks.\n";
        fs::remove_all(root, ec);
        return 1;
    }

    const fs::path mismatched_audio_path = root / "Suede - Beautiful Ones.mp3";
    {
        const std::vector<std::string> audio_args{
            "-loglevel", "error",
            "-nostdin",
            "-f", "lavfi",
            "-i", "sine=frequency=660:duration=1",
            "-c:a", "libmp3lame",
            "-metadata", "title=Beautiful Ones",
            "-metadata", "artist=Suede",
            "-metadata", "lyrics=Reamonn - The Only Ones",
            "-y",
            mismatched_audio_path.string()};
        if (!test_media_fixture::runCommand(*ffmpeg_path, audio_args) || !fs::exists(mismatched_audio_path)) {
            std::cerr << "Failed to generate mismatched lyrics fixture.\n";
            fs::remove_all(root, ec);
            return 1;
        }
    }

    auto offline_lyrics_provider = lofibox::platform::host::createHostLyricsProvider(
        std::make_shared<lofibox::platform::host::runtime_detail::SharedRuntimeCache>(),
        std::make_shared<OfflineConnectivityProvider>(),
        nullptr,
        nullptr);
    lofibox::app::TrackMetadata mismatched_metadata{};
    mismatched_metadata.title = "Beautiful Ones";
    mismatched_metadata.artist = "Suede";
    const auto mismatched_lyrics = offline_lyrics_provider->fetch(mismatched_audio_path, mismatched_metadata);
    if (mismatched_lyrics.plain || mismatched_lyrics.synced) {
        std::cerr << "Expected mismatched embedded lyrics to be rejected.\n";
        fs::remove_all(root, ec);
        return 1;
    }

    fs::remove_all(root, ec);
    return 0;
}
