// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/library_navigation_service.h"

#include "app/library_query_service.h"

namespace lofibox::app {

std::optional<std::string> LibraryNavigationService::titleOverrideForPage(
    AppPage page,
    const LibraryListContext& context)
{
    if (page == AppPage::Songs && !context.songs.label.empty()) {
        return context.songs.label;
    }
    if (page == AppPage::PlaylistDetail && !context.playlist.name.empty()) {
        return context.playlist.name;
    }
    return std::nullopt;
}

std::optional<std::vector<std::pair<std::string, std::string>>> LibraryNavigationService::rowsForPage(
    AppPage page,
    const LibraryModel& library,
    const LibraryListContext& context,
    const std::vector<AlbumRecord>& visible_albums,
    const std::vector<int>& playlist_track_ids)
{
    std::vector<std::pair<std::string, std::string>> rows{};
    switch (page) {
    case AppPage::MusicIndex:
        return std::vector<std::pair<std::string, std::string>>{
            {"ARTISTS", std::to_string(library.artists.size())},
            {"ALBUMS", std::to_string(library.albums.size())},
            {"SONGS", std::to_string(library.tracks.size())},
            {"GENRES", std::to_string(library.genres.size())},
            {"COMPOSERS", std::to_string(library.composers.size())},
            {"COMPILATIONS", std::to_string(library.compilations.size())},
        };
    case AppPage::Artists:
        for (const auto& artist : library.artists) rows.emplace_back(artist, "");
        return rows;
    case AppPage::Albums:
        for (const auto& album : visible_albums) rows.emplace_back(album.album, album.artist);
        return rows;
    case AppPage::Songs:
        for (const int id : context.songs.track_ids) {
            if (const auto* track = LibraryQueryService::findTrack(library, id)) rows.emplace_back(track->title, track->artist);
        }
        return rows;
    case AppPage::Genres:
        for (const auto& genre : library.genres) rows.emplace_back(genre, "");
        return rows;
    case AppPage::Composers:
        for (const auto& composer : library.composers) rows.emplace_back(composer, "");
        return rows;
    case AppPage::Compilations:
        for (const auto& compilation : library.compilations) rows.emplace_back(compilation.album, std::to_string(compilation.track_ids.size()));
        return rows;
    case AppPage::Playlists:
        return std::vector<std::pair<std::string, std::string>>{
            {"ON THE GO", std::to_string(context.on_the_go_ids.size())},
            {"RECENTLY ADDED", "AUTO"},
            {"MOST PLAYED", "AUTO"},
            {"RECENTLY PLAYED", "AUTO"},
        };
    case AppPage::PlaylistDetail:
        for (const int id : playlist_track_ids) {
            if (const auto* track = LibraryQueryService::findTrack(library, id)) rows.emplace_back(track->title, track->artist);
        }
        return rows;
    default:
        return std::nullopt;
    }
}

} // namespace lofibox::app
