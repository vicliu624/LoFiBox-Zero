// SPDX-License-Identifier: GPL-3.0-or-later

#include "library/library_store.h"

#include <cassert>
#include <filesystem>
#include <string>

int main()
{
    const auto root = std::filesystem::temp_directory_path() / "lofibox_library_store_smoke";
    std::error_code ec{};
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);

    lofibox::library::LibraryStore store{root / "library.tsv"};
    assert(store.metadata().schema_version == 2);

    lofibox::app::LibraryModel model{};
    lofibox::app::TrackRecord track{};
    track.id = 9;
    track.path = root / "Artist" / "Stored Song.flac";
    track.title = "Stored 曲目";
    track.artist = "Stored Artist";
    track.album = "Stored Album";
    track.genre = "Ambient";
    track.composer = "Stored Composer";
    track.added_time = 1'725'000'000;
    track.duration_seconds = 271;
    track.play_count = 4;
    track.last_played = 1'725'100'000;
    track.file_size_bytes = 123'456U;
    track.file_mtime_ticks = -987'654;
    track.fingerprint = "local-fingerprint";
    model.tracks.push_back(track);

    // Remote records are not local-library index entries and must not be
    // persisted as if they were local files.
    auto remote = track;
    remote.remote = true;
    remote.id = 10;
    model.tracks.push_back(std::move(remote));

    assert(store.save(model));
    const auto loaded = store.load();
    assert(loaded.tracks.size() == 1U);
    const auto& restored = loaded.tracks.front();
    assert(restored.id == 9);
    assert(restored.path == track.path);
    assert(restored.title == "Stored 曲目");
    assert(restored.artist == "Stored Artist");
    assert(restored.album == "Stored Album");
    assert(restored.genre == "Ambient");
    assert(restored.composer == "Stored Composer");
    assert(restored.added_time == 1'725'000'000);
    assert(restored.duration_seconds == 271);
    assert(restored.play_count == 4);
    assert(restored.last_played == 1'725'100'000);
    assert(restored.file_size_bytes == 123'456U);
    assert(restored.file_mtime_ticks == -987'654);
    assert(restored.fingerprint == "local-fingerprint");

    std::filesystem::remove_all(root, ec);
    return 0;
}
