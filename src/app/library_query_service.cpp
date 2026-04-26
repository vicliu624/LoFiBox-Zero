// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/library_query_service.h"

#include <algorithm>
#include <cctype>

namespace lofibox::app {
namespace {

std::string sortKey(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool trackLess(const TrackRecord& left, const TrackRecord& right, int lhs, int rhs, SongSortMode sort_mode)
{
    switch (sort_mode) {
    case SongSortMode::TitleAscending:
        return sortKey(left.title) == sortKey(right.title) ? lhs < rhs : sortKey(left.title) < sortKey(right.title);
    case SongSortMode::TitleDescending:
        return sortKey(left.title) == sortKey(right.title) ? lhs < rhs : sortKey(left.title) > sortKey(right.title);
    case SongSortMode::ArtistAscending:
        return sortKey(left.artist) == sortKey(right.artist) ? sortKey(left.title) < sortKey(right.title) : sortKey(left.artist) < sortKey(right.artist);
    case SongSortMode::ArtistDescending:
        return sortKey(left.artist) == sortKey(right.artist) ? sortKey(left.title) < sortKey(right.title) : sortKey(left.artist) > sortKey(right.artist);
    case SongSortMode::AddedAscending:
        return left.added_time == right.added_time ? sortKey(left.title) < sortKey(right.title) : left.added_time < right.added_time;
    case SongSortMode::AddedDescending:
        return left.added_time == right.added_time ? sortKey(left.title) < sortKey(right.title) : left.added_time > right.added_time;
    }
    return lhs < rhs;
}

} // namespace

const TrackRecord* LibraryQueryService::findTrack(const LibraryModel& library, int id) noexcept
{
    for (const auto& track : library.tracks) {
        if (track.id == id) {
            return &track;
        }
    }
    return nullptr;
}

TrackRecord* LibraryQueryService::findMutableTrack(LibraryModel& library, int id) noexcept
{
    for (auto& track : library.tracks) {
        if (track.id == id) {
            return &track;
        }
    }
    return nullptr;
}

std::vector<int> LibraryQueryService::sortedTrackIds(const LibraryModel& library, SongSortMode sort_mode)
{
    std::vector<int> ids{};
    ids.reserve(library.tracks.size());
    for (const auto& track : library.tracks) {
        ids.push_back(track.id);
    }
    std::sort(ids.begin(), ids.end(), [&](int lhs, int rhs) {
        const auto* left = findTrack(library, lhs);
        const auto* right = findTrack(library, rhs);
        if (!left || !right) {
            return lhs < rhs;
        }
        return trackLess(*left, *right, lhs, rhs, sort_mode);
    });
    return ids;
}

std::vector<int> LibraryQueryService::idsForGenre(const LibraryModel& library, const std::string& genre)
{
    std::vector<int> ids{};
    for (const auto& track : library.tracks) {
        if (track.genre == genre) {
            ids.push_back(track.id);
        }
    }
    return ids;
}

std::vector<int> LibraryQueryService::idsForComposer(const LibraryModel& library, const std::string& composer)
{
    std::vector<int> ids{};
    for (const auto& track : library.tracks) {
        if (track.composer == composer) {
            ids.push_back(track.id);
        }
    }
    return ids;
}

std::vector<AlbumRecord> LibraryQueryService::visibleAlbums(const LibraryModel& library, const AlbumsContext& context)
{
    std::vector<AlbumRecord> visible{};
    for (const auto& album : library.albums) {
        if (context.mode != AlbumsMode::ByArtist || album.artist == context.artist) {
            visible.push_back(album);
        }
    }
    return visible;
}

} // namespace lofibox::app
