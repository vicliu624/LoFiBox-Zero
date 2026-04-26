// SPDX-License-Identifier: GPL-3.0-or-later

#include "library/library_store.h"

#include <cassert>
#include <filesystem>

int main()
{
    const auto root = std::filesystem::temp_directory_path() / "lofibox_library_store_smoke";
    std::error_code ec{};
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);

    lofibox::library::LibraryStore store{root / "library.tsv"};
    assert(store.metadata().schema_version == 1);

    lofibox::app::LibraryModel model{};
    lofibox::app::TrackRecord track{};
    track.id = 9;
    track.title = "Stored Song";
    track.artist = "Stored Artist";
    model.tracks.push_back(track);

    assert(store.save(model));
    const auto loaded = store.load();
    assert(loaded.tracks.size() == 1U);
    assert(loaded.tracks.front().id == 9);
    assert(loaded.tracks.front().title == "Stored Song");

    std::filesystem::remove_all(root, ec);
    return 0;
}
