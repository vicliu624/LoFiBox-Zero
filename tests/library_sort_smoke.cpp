// SPDX-License-Identifier: GPL-3.0-or-later
#include "application/library_query_service.h"
#include <algorithm>
#include <cassert>
#include <cctype>
#include <chrono>
#include <iostream>
#include <numeric>
#include <random>

using namespace lofibox;
std::string lower(std::string text)
{
    for (auto& ch : text) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return text;
}

int main(int argc, char** argv)
{
    app::LibraryModel model;
    std::mt19937 random{19};
    for (int id = 1; id <= 250; ++id) {
        app::TrackRecord track;
        track.id = id;
        track.title = (id % 2 ? "Track " : "TRACK ") + std::to_string(random() % 40);
        track.artist = (id % 3 ? "Artist " : "ARTIST ") + std::to_string(random() % 10);
        track.added_time = random() % 5;
        model.tracks.push_back(std::move(track));
    }
    std::shuffle(model.tracks.begin(), model.tracks.end(), random);
    for (int mode = 0; mode < 6; ++mode) {
        std::vector<int> expected;
        for (const auto& track : model.tracks) expected.push_back(track.id);
        // Preserve the previous sort contract, including case folding, title
        // tie order in artist/date modes, and ID tie order in title modes.
        std::sort(expected.begin(), expected.end(), [&](int a, int b) {
            const auto& left = *application::LibraryQueryService::findTrack(model, a);
            const auto& right = *application::LibraryQueryService::findTrack(model, b);
            const auto lt = lower(left.title), rt = lower(right.title);
            const auto la = lower(left.artist), ra = lower(right.artist);
            switch (static_cast<app::SongSortMode>(mode)) {
            case app::SongSortMode::TitleAscending: return lt == rt ? a < b : lt < rt;
            case app::SongSortMode::TitleDescending: return lt == rt ? a < b : lt > rt;
            case app::SongSortMode::ArtistAscending: return la == ra ? lt < rt : la < ra;
            case app::SongSortMode::ArtistDescending: return la == ra ? lt < rt : la > ra;
            case app::SongSortMode::AddedAscending: return left.added_time == right.added_time ? lt < rt : left.added_time < right.added_time;
            case app::SongSortMode::AddedDescending: return left.added_time == right.added_time ? lt < rt : left.added_time > right.added_time;
            }
            return a < b;
        });
        assert(application::LibraryQueryService::sortedTrackIds(model, static_cast<app::SongSortMode>(mode)) == expected);
    }
    assert(application::LibraryQueryService::sortedTrackIds({}, app::SongSortMode::TitleAscending).empty());
    if (argc > 1 && std::string_view{argv[1]} == "--benchmark") {
        for (int size : {1000, 10000, 50000}) {
            model.tracks.clear();
            for (int id = 1; id <= size; ++id) {
                app::TrackRecord track;
                track.id = id;
                track.title = "Track " + std::to_string(size - id + 1);
                model.tracks.push_back(std::move(track));
            }
            const auto start = std::chrono::steady_clock::now();
            const auto ids = application::LibraryQueryService::sortedTrackIds(model, app::SongSortMode::TitleAscending);
            assert(ids.size() == static_cast<std::size_t>(size));
            std::cout << size << " tracks: " << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() << " ms\n";
        }
    }
}
