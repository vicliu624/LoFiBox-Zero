// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/library_navigation_service.h"

#include "application/library_query_service.h"
#include <unordered_map>

namespace lofibox::app {
namespace {

std::vector<std::pair<std::string, std::string>> groupedRows(
    const LibraryModel& library, const std::vector<std::string>& groups,
    const std::string TrackRecord::* field)
{
    std::unordered_map<std::string_view, int> counts;
    counts.reserve(groups.size());
    for (const auto& track : library.tracks) {
        ++counts[track.*field];
    }
    std::vector<std::pair<std::string, std::string>> rows;
    rows.reserve(groups.size());
    for (const auto& group : groups) {
        const auto it = counts.find(group);
        rows.emplace_back(group, std::to_string(it == counts.end() ? 0 : it->second));
    }
    return rows;
}

std::vector<std::pair<std::string, std::string>> trackRows(const LibraryModel& library, const std::vector<int>& ids)
{
    std::unordered_map<int, const TrackRecord*> tracks;
    tracks.reserve(library.tracks.size());
    for (const auto& track : library.tracks) tracks.emplace(track.id, &track);
    std::vector<std::pair<std::string, std::string>> rows;
    rows.reserve(ids.size());
    for (int id : ids) {
        if (const auto it = tracks.find(id); it != tracks.end()) {
            rows.emplace_back(it->second->title, it->second->artist);
        }
    }
    return rows;
}

} // namespace

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
        return groupedRows(library, library.artists, &TrackRecord::artist);
    case AppPage::Albums:
        for (const auto& album : visible_albums) rows.emplace_back(album.album, std::to_string(album.track_ids.size()));
        return rows;
    case AppPage::Songs:
        return trackRows(library, context.songs.track_ids);
    case AppPage::Genres:
        return groupedRows(library, library.genres, &TrackRecord::genre);
    case AppPage::Composers:
        return groupedRows(library, library.composers, &TrackRecord::composer);
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
        return trackRows(library, playlist_track_ids);
    default:
        return std::nullopt;
    }
}

} // namespace lofibox::app
