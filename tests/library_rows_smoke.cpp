// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/library_controller.h"
#include "app/library_navigation_service.h"
#include "app/library_scanner.h"
#include <cassert>

using namespace lofibox::app;

int main()
{
    LibraryModel model;
    model.tracks = {
        {.id = 5, .title = "B", .artist = "Two", .genre = "Rock", .composer = "X", .added_time = 5000, .play_count = 1, .last_played = 10},
        {.id = 9, .title = "A", .artist = "One", .genre = "Rock", .composer = "Y", .added_time = 100, .play_count = 3},
        {.id = 22, .title = "C", .artist = "Two", .genre = "Jazz", .composer = "X", .added_time = 200, .play_count = 3, .last_played = 20},
    };
    rebuildLibraryIndexes(model);
    LibraryListContext context;
    context.songs.track_ids = {22, 5, -1, 22, 9};
    const std::vector<std::pair<std::string, std::string>> expected{{"C", "Two"}, {"B", "Two"}, {"C", "Two"}, {"A", "One"}};
    const auto songs = LibraryNavigationService::rowsForPage(AppPage::Songs, model, context, {}, {});
    assert(songs && *songs == expected);
    const auto playlist = LibraryNavigationService::rowsForPage(AppPage::PlaylistDetail, model, context, {}, context.songs.track_ids);
    assert(playlist == songs);
    model.artists.push_back("No tracks");
    const auto artists = LibraryNavigationService::rowsForPage(AppPage::Artists, model, context, {}, {});
    assert(artists && artists->size() == 3);
    assert((*artists)[0] == std::make_pair(std::string{"One"}, std::string{"1"}));
    assert((*artists)[1] == std::make_pair(std::string{"Two"}, std::string{"2"}));
    assert(artists->back().second == "0");
    const auto genres = LibraryNavigationService::rowsForPage(AppPage::Genres, model, context, {}, {});
    const auto composers = LibraryNavigationService::rowsForPage(AppPage::Composers, model, context, {}, {});
    assert(genres && (*genres)[0].first == "Jazz" && (*genres)[0].second == "1" && (*genres)[1].second == "2");
    assert(composers && (*composers)[0].first == "X" && (*composers)[0].second == "2" && (*composers)[1].second == "1");
    assert(!LibraryNavigationService::rowsForPage(AppPage::NowPlaying, model, context, {}, {}));

    LibraryController controller;
    controller.mutableModel() = model;
    controller.setSongsContextAll();
    const std::vector<std::vector<int>> ordered{{}, {5, 22, 9}, {9, 22, 5}, {22, 5}};
    for (int kind = 0; kind < 4; ++kind) {
        (void)controller.openSelectedListItem(AppPage::Playlists, kind);
        assert(controller.playlistTrackIds() == ordered[kind]);
        const auto detail = controller.rowsForPage(AppPage::PlaylistDetail);
        assert(detail && detail->size() == ordered[kind].size());
        const auto unrelated = controller.rowsForPage(AppPage::Songs);
        assert(unrelated && unrelated->size() == 3 && unrelated->front().first == "A");
    }

    // Distinct artists exercise the high-cardinality case rather than a small
    // fixed artist set that hides repeated full-library scans.
    model = {};
    for (int id = 1; id <= 10000; ++id) {
        TrackRecord track;
        track.id = id;
        track.title = std::to_string(id);
        track.artist = "Artist " + std::to_string(id);
        model.artists.push_back(track.artist);
        model.tracks.push_back(std::move(track));
    }
    const auto many = LibraryNavigationService::rowsForPage(AppPage::Artists, model, {}, {}, {});
    assert(many && many->size() == 10000);
    for (const auto& row : *many) assert(row.second == "1");
}
