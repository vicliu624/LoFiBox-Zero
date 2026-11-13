// SPDX-License-Identifier: GPL-3.0-or-later

#include "application/library_query_service.h"

#include <algorithm>
#include <cctype>

#include "app/library_controller.h"
#include "app/media_search_service.h"

namespace lofibox::application {
namespace {

std::string sortKey(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

struct SortEntry {
    const app::TrackRecord* track{};
    std::string title{};
    std::string artist{};
};

bool trackLess(const SortEntry& left, const SortEntry& right, app::SongSortMode sort_mode)
{
    switch (sort_mode) {
    case app::SongSortMode::TitleAscending:
        return left.title == right.title ? left.track->id < right.track->id : left.title < right.title;
    case app::SongSortMode::TitleDescending:
        return left.title == right.title ? left.track->id < right.track->id : left.title > right.title;
    case app::SongSortMode::ArtistAscending:
        return left.artist == right.artist ? left.title < right.title : left.artist < right.artist;
    case app::SongSortMode::ArtistDescending:
        return left.artist == right.artist ? left.title < right.title : left.artist > right.artist;
    case app::SongSortMode::AddedAscending:
        return left.track->added_time == right.track->added_time ? left.title < right.title : left.track->added_time < right.track->added_time;
    case app::SongSortMode::AddedDescending:
        return left.track->added_time == right.track->added_time ? left.title < right.title : left.track->added_time > right.track->added_time;
    }
    return left.track->id < right.track->id;
}

} // namespace

LibraryQueryService::LibraryQueryService(const app::LibraryController& controller) noexcept
    : controller_(controller)
{
}

std::unique_lock<std::recursive_mutex> LibraryQueryService::lockState() const
{
    return std::unique_lock{controller_.stateMutex()};
}

app::LibraryIndexState LibraryQueryService::state() const noexcept
{
    return controller_.state();
}

const app::LibraryModel& LibraryQueryService::model() const noexcept
{
    return controller_.model();
}

app::LibraryScanProgress LibraryQueryService::scanProgress() const
{
    return controller_.scanProgress();
}

const app::TrackRecord* LibraryQueryService::findTrack(int id) const noexcept
{
    return controller_.findTrack(id);
}

std::vector<int> LibraryQueryService::allSongIdsSorted() const
{
    return controller_.allSongIdsSorted();
}

std::vector<int> LibraryQueryService::trackIdsForCurrentSongs() const
{
    return controller_.trackIdsForCurrentSongs();
}

std::vector<int> LibraryQueryService::playlistTrackIds() const
{
    return controller_.playlistTrackIds();
}

app::SongSortMode LibraryQueryService::songSortMode() const noexcept
{
    return controller_.songSortMode();
}

std::string LibraryQueryService::songSortLabel() const
{
    return controller_.songSortLabel();
}

std::optional<std::string> LibraryQueryService::titleOverrideForPage(app::AppPage page) const
{
    return controller_.titleOverrideForPage(page);
}

std::optional<std::vector<std::pair<std::string, std::string>>> LibraryQueryService::rowsForPage(app::AppPage page) const
{
    return controller_.rowsForPage(page);
}

std::vector<app::MediaItem> LibraryQueryService::searchLocal(std::string_view query, int limit) const
{
    return app::MediaSearchService::searchLocal(controller_.model(), query, limit).local_items;
}

const app::TrackRecord* LibraryQueryService::findTrack(const app::LibraryModel& library, int id) noexcept
{
    for (const auto& track : library.tracks) {
        if (track.id == id) {
            return &track;
        }
    }
    return nullptr;
}

app::TrackRecord* LibraryQueryService::findMutableTrack(app::LibraryModel& library, int id) noexcept
{
    for (auto& track : library.tracks) {
        if (track.id == id) {
            return &track;
        }
    }
    return nullptr;
}

std::vector<int> LibraryQueryService::sortedTrackIds(const app::LibraryModel& library, app::SongSortMode sort_mode)
{
    std::vector<SortEntry> entries;
    entries.reserve(library.tracks.size());
    const bool by_artist = sort_mode == app::SongSortMode::ArtistAscending || sort_mode == app::SongSortMode::ArtistDescending;
    for (const auto& track : library.tracks) {
        entries.push_back({&track, sortKey(track.title), by_artist ? sortKey(track.artist) : std::string{}});
    }
    std::sort(entries.begin(), entries.end(), [sort_mode](const SortEntry& left, const SortEntry& right) {
        return trackLess(left, right, sort_mode);
    });
    std::vector<int> ids;
    ids.reserve(entries.size());
    for (const auto& entry : entries) ids.push_back(entry.track->id);
    return ids;
}

std::vector<int> LibraryQueryService::idsForGenre(const app::LibraryModel& library, const std::string& genre)
{
    std::vector<int> ids{};
    for (const auto& track : library.tracks) {
        if (track.genre == genre) {
            ids.push_back(track.id);
        }
    }
    return ids;
}

std::vector<int> LibraryQueryService::idsForComposer(const app::LibraryModel& library, const std::string& composer)
{
    std::vector<int> ids{};
    for (const auto& track : library.tracks) {
        if (track.composer == composer) {
            ids.push_back(track.id);
        }
    }
    return ids;
}

std::vector<app::AlbumRecord> LibraryQueryService::visibleAlbums(const app::LibraryModel& library, const app::AlbumsContext& context)
{
    std::vector<app::AlbumRecord> visible{};
    for (const auto& album : library.albums) {
        if (context.mode != app::AlbumsMode::ByArtist || album.artist == context.artist) {
            visible.push_back(album);
        }
    }
    return visible;
}

} // namespace lofibox::application
