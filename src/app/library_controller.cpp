// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/library_controller.h"

#include <algorithm>
#include <cctype>
#include <cstddef>

#include "app/library_navigation_service.h"
#include "app/library_open_action_resolver.h"
#include "app/library_query_service.h"
#include "app/library_scanner.h"

namespace lofibox::app {
namespace {

std::string sortKey(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

} // namespace

LibraryIndexState LibraryController::state() const noexcept
{
    return state_;
}

const LibraryModel& LibraryController::model() const noexcept
{
    return library_;
}

LibraryModel& LibraryController::mutableModel() noexcept
{
    return library_;
}

void LibraryController::startLoading() noexcept
{
    state_ = LibraryIndexState::Loading;
}

void LibraryController::refreshLibrary(const std::vector<std::filesystem::path>& media_roots, const MetadataProvider& metadata_provider)
{
    library_ = scanLibrary(media_roots, metadata_provider);
    state_ = library_.degraded ? LibraryIndexState::Degraded : LibraryIndexState::Ready;
    setSongsContextAll();
}

const TrackRecord* LibraryController::findTrack(int id) const noexcept
{
    return LibraryQueryService::findTrack(library_, id);
}

TrackRecord* LibraryController::findMutableTrack(int id) noexcept
{
    return LibraryQueryService::findMutableTrack(library_, id);
}

std::vector<int> LibraryController::allSongIdsSorted() const
{
    return LibraryQueryService::sortedTrackIds(library_, list_context_.song_sort_mode);
}

std::vector<int> LibraryController::trackIdsForCurrentSongs() const
{
    return list_context_.songs.track_ids;
}

std::vector<int> LibraryController::playlistTrackIds() const
{
    std::vector<int> ids{};
    switch (list_context_.playlist.kind) {
    case PlaylistKind::OnTheGo:
        ids = list_context_.on_the_go_ids;
        break;
    case PlaylistKind::RecentlyAdded:
        ids = allSongIdsSorted();
        std::sort(ids.begin(), ids.end(), [this](int lhs, int rhs) {
            const auto* left = findTrack(lhs);
            const auto* right = findTrack(rhs);
            if (!left || !right) {
                return lhs < rhs;
            }
            return left->added_time > right->added_time;
        });
        break;
    case PlaylistKind::MostPlayed:
        ids = allSongIdsSorted();
        std::sort(ids.begin(), ids.end(), [this](int lhs, int rhs) {
            const auto* left = findTrack(lhs);
            const auto* right = findTrack(rhs);
            if (!left || !right) {
                return lhs < rhs;
            }
            if (left->play_count != right->play_count) {
                return left->play_count > right->play_count;
            }
            return left->title < right->title;
        });
        break;
    case PlaylistKind::RecentlyPlayed:
        ids = allSongIdsSorted();
        std::sort(ids.begin(), ids.end(), [this](int lhs, int rhs) {
            const auto* left = findTrack(lhs);
            const auto* right = findTrack(rhs);
            if (!left || !right) {
                return lhs < rhs;
            }
            if (left->last_played != right->last_played) {
                return left->last_played > right->last_played;
            }
            return left->title < right->title;
        });
        ids.erase(std::remove_if(ids.begin(), ids.end(), [this](int id) {
            const auto* track = findTrack(id);
            return !track || track->last_played == 0;
        }), ids.end());
        break;
    }
    return ids;
}

SongSortMode LibraryController::songSortMode() const noexcept
{
    return list_context_.song_sort_mode;
}

std::string LibraryController::songSortLabel() const
{
    switch (list_context_.song_sort_mode) {
    case SongSortMode::TitleAscending: return "TITLE A-Z";
    case SongSortMode::TitleDescending: return "TITLE Z-A";
    case SongSortMode::ArtistAscending: return "ARTIST A-Z";
    case SongSortMode::ArtistDescending: return "ARTIST Z-A";
    case SongSortMode::AddedAscending: return "ADDED OLD";
    case SongSortMode::AddedDescending: return "ADDED NEW";
    }
    return "TITLE A-Z";
}

void LibraryController::cycleSongSortMode()
{
    const int next = (static_cast<int>(list_context_.song_sort_mode) + 1) % 6;
    list_context_.song_sort_mode = static_cast<SongSortMode>(next);
    switch (list_context_.songs.mode) {
    case SongsMode::All:
        list_context_.songs.track_ids = allSongIdsSorted();
        break;
    case SongsMode::Album:
    case SongsMode::Genre:
    case SongsMode::Composer:
    case SongsMode::Compilation:
        std::sort(list_context_.songs.track_ids.begin(), list_context_.songs.track_ids.end(), [this](int lhs, int rhs) {
            const auto* left = findTrack(lhs);
            const auto* right = findTrack(rhs);
            if (!left || !right) {
                return lhs < rhs;
            }
            switch (list_context_.song_sort_mode) {
            case SongSortMode::TitleAscending:
                return sortKey(left->title) == sortKey(right->title) ? lhs < rhs : sortKey(left->title) < sortKey(right->title);
            case SongSortMode::TitleDescending:
                return sortKey(left->title) == sortKey(right->title) ? lhs < rhs : sortKey(left->title) > sortKey(right->title);
            case SongSortMode::ArtistAscending:
                return sortKey(left->artist) == sortKey(right->artist) ? sortKey(left->title) < sortKey(right->title) : sortKey(left->artist) < sortKey(right->artist);
            case SongSortMode::ArtistDescending:
                return sortKey(left->artist) == sortKey(right->artist) ? sortKey(left->title) < sortKey(right->title) : sortKey(left->artist) > sortKey(right->artist);
            case SongSortMode::AddedAscending:
                return left->added_time == right->added_time ? sortKey(left->title) < sortKey(right->title) : left->added_time < right->added_time;
            case SongSortMode::AddedDescending:
                return left->added_time == right->added_time ? sortKey(left->title) < sortKey(right->title) : left->added_time > right->added_time;
            }
            return lhs < rhs;
        });
        break;
    }
}

void LibraryController::setSongsContextAll()
{
    list_context_.songs.mode = SongsMode::All;
    list_context_.songs.label = "SONGS";
    list_context_.songs.track_ids = allSongIdsSorted();
}

std::optional<std::string> LibraryController::titleOverrideForPage(AppPage page) const
{
    return LibraryNavigationService::titleOverrideForPage(page, list_context_);
}

std::optional<std::vector<std::pair<std::string, std::string>>> LibraryController::rowsForPage(AppPage page) const
{
    return LibraryNavigationService::rowsForPage(page, library_, list_context_, visibleAlbums(), playlistTrackIds());
}

LibraryOpenResult LibraryController::openSelectedListItem(AppPage page, int selected)
{
    return LibraryOpenActionResolver::openSelectedListItem(*this, page, selected);
}
void LibraryController::setSongsContextAlbum(const AlbumRecord& album)
{
    list_context_.songs.mode = SongsMode::Album;
    list_context_.songs.label = album.album;
    list_context_.songs.track_ids = album.track_ids;
    std::sort(list_context_.songs.track_ids.begin(), list_context_.songs.track_ids.end(), [this](int lhs, int rhs) {
        const auto* left = findTrack(lhs);
        const auto* right = findTrack(rhs);
        return left && right ? left->title < right->title : lhs < rhs;
    });
}

void LibraryController::setSongsContextFiltered(SongsMode mode, std::string label, std::vector<int> ids)
{
    list_context_.songs.mode = mode;
    list_context_.songs.label = std::move(label);
    list_context_.songs.track_ids = std::move(ids);
    std::sort(list_context_.songs.track_ids.begin(), list_context_.songs.track_ids.end(), [this](int lhs, int rhs) {
        const auto* left = findTrack(lhs);
        const auto* right = findTrack(rhs);
        return left && right ? left->title < right->title : lhs < rhs;
    });
}

std::vector<AlbumRecord> LibraryController::visibleAlbums() const
{
    return LibraryQueryService::visibleAlbums(library_, list_context_.albums);
}

std::vector<int> LibraryController::idsForGenre(const std::string& genre) const
{
    return LibraryQueryService::idsForGenre(library_, genre);
}

std::vector<int> LibraryController::idsForComposer(const std::string& composer) const
{
    return LibraryQueryService::idsForComposer(library_, composer);
}

} // namespace lofibox::app
