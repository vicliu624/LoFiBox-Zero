// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/library_controller.h"

#include <algorithm>
#include <cctype>
#include <cstddef>

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
    for (const auto& track : library_.tracks) {
        if (track.id == id) {
            return &track;
        }
    }
    return nullptr;
}

TrackRecord* LibraryController::findMutableTrack(int id) noexcept
{
    for (auto& track : library_.tracks) {
        if (track.id == id) {
            return &track;
        }
    }
    return nullptr;
}

std::vector<int> LibraryController::allSongIdsSorted() const
{
    std::vector<int> ids{};
    ids.reserve(library_.tracks.size());
    for (const auto& track : library_.tracks) {
        ids.push_back(track.id);
    }
    std::sort(ids.begin(), ids.end(), [this](int lhs, int rhs) {
        const auto* left = findTrack(lhs);
        const auto* right = findTrack(rhs);
        if (!left || !right) {
            return lhs < rhs;
        }
        switch (song_sort_mode_) {
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
    return ids;
}

std::vector<int> LibraryController::trackIdsForCurrentSongs() const
{
    return songs_context_.track_ids;
}

std::vector<int> LibraryController::playlistTrackIds() const
{
    std::vector<int> ids{};
    switch (playlist_context_.kind) {
    case PlaylistKind::OnTheGo:
        ids = on_the_go_ids_;
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
    return song_sort_mode_;
}

std::string LibraryController::songSortLabel() const
{
    switch (song_sort_mode_) {
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
    const int next = (static_cast<int>(song_sort_mode_) + 1) % 6;
    song_sort_mode_ = static_cast<SongSortMode>(next);
    switch (songs_context_.mode) {
    case SongsMode::All:
        songs_context_.track_ids = allSongIdsSorted();
        break;
    case SongsMode::Album:
    case SongsMode::Genre:
    case SongsMode::Composer:
    case SongsMode::Compilation:
        std::sort(songs_context_.track_ids.begin(), songs_context_.track_ids.end(), [this](int lhs, int rhs) {
            const auto* left = findTrack(lhs);
            const auto* right = findTrack(rhs);
            if (!left || !right) {
                return lhs < rhs;
            }
            switch (song_sort_mode_) {
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
    songs_context_.mode = SongsMode::All;
    songs_context_.label = "SONGS";
    songs_context_.track_ids = allSongIdsSorted();
}

std::optional<std::string> LibraryController::titleOverrideForPage(AppPage page) const
{
    if (page == AppPage::Songs && !songs_context_.label.empty()) {
        return songs_context_.label;
    }
    if (page == AppPage::PlaylistDetail && !playlist_context_.name.empty()) {
        return playlist_context_.name;
    }
    return std::nullopt;
}

std::optional<std::vector<std::pair<std::string, std::string>>> LibraryController::rowsForPage(AppPage page) const
{
    std::vector<std::pair<std::string, std::string>> rows{};
    switch (page) {
    case AppPage::MusicIndex:
        return std::vector<std::pair<std::string, std::string>>{
            {"ARTISTS", std::to_string(library_.artists.size())},
            {"ALBUMS", std::to_string(library_.albums.size())},
            {"SONGS", std::to_string(library_.tracks.size())},
            {"GENRES", std::to_string(library_.genres.size())},
            {"COMPOSERS", std::to_string(library_.composers.size())},
            {"COMPILATIONS", std::to_string(library_.compilations.size())},
        };
    case AppPage::Artists:
        for (const auto& artist : library_.artists) rows.emplace_back(artist, "");
        return rows;
    case AppPage::Albums:
        for (const auto& album : visibleAlbums()) rows.emplace_back(album.album, album.artist);
        return rows;
    case AppPage::Songs:
        for (const int id : songs_context_.track_ids) {
            if (const auto* track = findTrack(id)) rows.emplace_back(track->title, track->artist);
        }
        return rows;
    case AppPage::Genres:
        for (const auto& genre : library_.genres) rows.emplace_back(genre, "");
        return rows;
    case AppPage::Composers:
        for (const auto& composer : library_.composers) rows.emplace_back(composer, "");
        return rows;
    case AppPage::Compilations:
        for (const auto& compilation : library_.compilations) rows.emplace_back(compilation.album, std::to_string(compilation.track_ids.size()));
        return rows;
    case AppPage::Playlists:
        return std::vector<std::pair<std::string, std::string>>{
            {"ON THE GO", std::to_string(on_the_go_ids_.size())},
            {"RECENTLY ADDED", "AUTO"},
            {"MOST PLAYED", "AUTO"},
            {"RECENTLY PLAYED", "AUTO"},
        };
    case AppPage::PlaylistDetail:
        for (const int id : playlistTrackIds()) {
            if (const auto* track = findTrack(id)) rows.emplace_back(track->title, track->artist);
        }
        return rows;
    default:
        return std::nullopt;
    }
}

LibraryOpenResult LibraryController::openSelectedListItem(AppPage page, int selected)
{
    switch (page) {
    case AppPage::MusicIndex:
        switch (selected) {
        case 0: return {LibraryOpenResult::Kind::PushPage, AppPage::Artists, 0};
        case 1:
            albums_context_ = {};
            return {LibraryOpenResult::Kind::PushPage, AppPage::Albums, 0};
        case 2:
            setSongsContextAll();
            return {LibraryOpenResult::Kind::PushPage, AppPage::Songs, 0};
        case 3: return {LibraryOpenResult::Kind::PushPage, AppPage::Genres, 0};
        case 4: return {LibraryOpenResult::Kind::PushPage, AppPage::Composers, 0};
        case 5: return {LibraryOpenResult::Kind::PushPage, AppPage::Compilations, 0};
        default: return {};
        }
    case AppPage::Artists:
        if (selected >= 0 && selected < static_cast<int>(library_.artists.size())) {
            albums_context_.mode = AlbumsMode::ByArtist;
            albums_context_.artist = library_.artists[static_cast<std::size_t>(selected)];
            return {LibraryOpenResult::Kind::PushPage, AppPage::Albums, 0};
        }
        return {};
    case AppPage::Albums: {
        const auto albums = visibleAlbums();
        if (selected >= 0 && selected < static_cast<int>(albums.size())) {
            setSongsContextAlbum(albums[static_cast<std::size_t>(selected)]);
            return {LibraryOpenResult::Kind::PushPage, AppPage::Songs, 0};
        }
        return {};
    }
    case AppPage::Songs: {
        const auto ids = trackIdsForCurrentSongs();
        if (selected >= 0 && selected < static_cast<int>(ids.size())) {
            return {LibraryOpenResult::Kind::StartTrack, AppPage::NowPlaying, ids[static_cast<std::size_t>(selected)]};
        }
        return {};
    }
    case AppPage::Genres:
        if (selected >= 0 && selected < static_cast<int>(library_.genres.size())) {
            const auto& genre = library_.genres[static_cast<std::size_t>(selected)];
            setSongsContextFiltered(SongsMode::Genre, genre, idsForGenre(genre));
            return {LibraryOpenResult::Kind::PushPage, AppPage::Songs, 0};
        }
        return {};
    case AppPage::Composers:
        if (selected >= 0 && selected < static_cast<int>(library_.composers.size())) {
            const auto& composer = library_.composers[static_cast<std::size_t>(selected)];
            setSongsContextFiltered(SongsMode::Composer, composer, idsForComposer(composer));
            return {LibraryOpenResult::Kind::PushPage, AppPage::Songs, 0};
        }
        return {};
    case AppPage::Compilations:
        if (selected >= 0 && selected < static_cast<int>(library_.compilations.size())) {
            const auto& compilation = library_.compilations[static_cast<std::size_t>(selected)];
            setSongsContextFiltered(SongsMode::Compilation, compilation.album, compilation.track_ids);
            return {LibraryOpenResult::Kind::PushPage, AppPage::Songs, 0};
        }
        return {};
    case AppPage::Playlists: {
        const int index = std::clamp(selected, 0, 3);
        playlist_context_.kind = static_cast<PlaylistKind>(index);
        if (const auto rows = rowsForPage(AppPage::Playlists); rows && index < static_cast<int>(rows->size())) {
            playlist_context_.name = (*rows)[static_cast<std::size_t>(index)].first;
        }
        return {LibraryOpenResult::Kind::PushPage, AppPage::PlaylistDetail, 0};
    }
    case AppPage::PlaylistDetail: {
        const auto ids = playlistTrackIds();
        if (selected >= 0 && selected < static_cast<int>(ids.size())) {
            return {LibraryOpenResult::Kind::StartTrack, AppPage::NowPlaying, ids[static_cast<std::size_t>(selected)]};
        }
        return {};
    }
    default:
        return {};
    }
}

void LibraryController::setSongsContextAlbum(const AlbumRecord& album)
{
    songs_context_.mode = SongsMode::Album;
    songs_context_.label = album.album;
    songs_context_.track_ids = album.track_ids;
    std::sort(songs_context_.track_ids.begin(), songs_context_.track_ids.end(), [this](int lhs, int rhs) {
        const auto* left = findTrack(lhs);
        const auto* right = findTrack(rhs);
        return left && right ? left->title < right->title : lhs < rhs;
    });
}

void LibraryController::setSongsContextFiltered(SongsMode mode, std::string label, std::vector<int> ids)
{
    songs_context_.mode = mode;
    songs_context_.label = std::move(label);
    songs_context_.track_ids = std::move(ids);
    std::sort(songs_context_.track_ids.begin(), songs_context_.track_ids.end(), [this](int lhs, int rhs) {
        const auto* left = findTrack(lhs);
        const auto* right = findTrack(rhs);
        return left && right ? left->title < right->title : lhs < rhs;
    });
}

std::vector<AlbumRecord> LibraryController::visibleAlbums() const
{
    std::vector<AlbumRecord> visible{};
    for (const auto& album : library_.albums) {
        if (albums_context_.mode != AlbumsMode::ByArtist || album.artist == albums_context_.artist) {
            visible.push_back(album);
        }
    }
    return visible;
}

std::vector<int> LibraryController::idsForGenre(const std::string& genre) const
{
    std::vector<int> ids{};
    for (const auto& track : library_.tracks) {
        if (track.genre == genre) {
            ids.push_back(track.id);
        }
    }
    return ids;
}

std::vector<int> LibraryController::idsForComposer(const std::string& composer) const
{
    std::vector<int> ids{};
    for (const auto& track : library_.tracks) {
        if (track.composer == composer) {
            ids.push_back(track.id);
        }
    }
    return ids;
}

} // namespace lofibox::app
