// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/library_open_action_resolver.h"

#include <algorithm>
#include <cstddef>

namespace lofibox::app {

LibraryOpenResult LibraryOpenActionResolver::openSelectedListItem(LibraryController& controller, AppPage page, int selected)
{
    switch (page) {
    case AppPage::MusicIndex:
        switch (selected) {
        case 0: return {LibraryOpenResult::Kind::PushPage, AppPage::Artists, 0};
        case 1:
            controller.list_context_.albums = {};
            return {LibraryOpenResult::Kind::PushPage, AppPage::Albums, 0};
        case 2:
            controller.setSongsContextAll();
            return {LibraryOpenResult::Kind::PushPage, AppPage::Songs, 0};
        case 3: return {LibraryOpenResult::Kind::PushPage, AppPage::Genres, 0};
        case 4: return {LibraryOpenResult::Kind::PushPage, AppPage::Composers, 0};
        case 5: return {LibraryOpenResult::Kind::PushPage, AppPage::Compilations, 0};
        default: return {};
        }
    case AppPage::Artists:
        if (selected >= 0 && selected < static_cast<int>(controller.library_.artists.size())) {
            controller.list_context_.albums.mode = AlbumsMode::ByArtist;
            controller.list_context_.albums.artist = controller.library_.artists[static_cast<std::size_t>(selected)];
            return {LibraryOpenResult::Kind::PushPage, AppPage::Albums, 0};
        }
        return {};
    case AppPage::Albums: {
        const auto albums = controller.visibleAlbums();
        if (selected >= 0 && selected < static_cast<int>(albums.size())) {
            controller.setSongsContextAlbum(albums[static_cast<std::size_t>(selected)]);
            return {LibraryOpenResult::Kind::PushPage, AppPage::Songs, 0};
        }
        return {};
    }
    case AppPage::Songs: {
        const auto ids = controller.trackIdsForCurrentSongs();
        if (selected >= 0 && selected < static_cast<int>(ids.size())) {
            return {LibraryOpenResult::Kind::StartTrack, AppPage::NowPlaying, ids[static_cast<std::size_t>(selected)]};
        }
        return {};
    }
    case AppPage::Genres:
        if (selected >= 0 && selected < static_cast<int>(controller.library_.genres.size())) {
            const auto& genre = controller.library_.genres[static_cast<std::size_t>(selected)];
            controller.setSongsContextFiltered(SongsMode::Genre, genre, controller.idsForGenre(genre));
            return {LibraryOpenResult::Kind::PushPage, AppPage::Songs, 0};
        }
        return {};
    case AppPage::Composers:
        if (selected >= 0 && selected < static_cast<int>(controller.library_.composers.size())) {
            const auto& composer = controller.library_.composers[static_cast<std::size_t>(selected)];
            controller.setSongsContextFiltered(SongsMode::Composer, composer, controller.idsForComposer(composer));
            return {LibraryOpenResult::Kind::PushPage, AppPage::Songs, 0};
        }
        return {};
    case AppPage::Compilations:
        if (selected >= 0 && selected < static_cast<int>(controller.library_.compilations.size())) {
            const auto& compilation = controller.library_.compilations[static_cast<std::size_t>(selected)];
            controller.setSongsContextFiltered(SongsMode::Compilation, compilation.album, compilation.track_ids);
            return {LibraryOpenResult::Kind::PushPage, AppPage::Songs, 0};
        }
        return {};
    case AppPage::Playlists: {
        const int index = std::clamp(selected, 0, 3);
        controller.list_context_.playlist.kind = static_cast<PlaylistKind>(index);
        if (const auto rows = controller.rowsForPage(AppPage::Playlists); rows && index < static_cast<int>(rows->size())) {
            controller.list_context_.playlist.name = (*rows)[static_cast<std::size_t>(index)].first;
        }
        return {LibraryOpenResult::Kind::PushPage, AppPage::PlaylistDetail, 0};
    }
    case AppPage::PlaylistDetail: {
        const auto ids = controller.playlistTrackIds();
        if (selected >= 0 && selected < static_cast<int>(ids.size())) {
            return {LibraryOpenResult::Kind::StartTrack, AppPage::NowPlaying, ids[static_cast<std::size_t>(selected)]};
        }
        return {};
    }
    default:
        return {};
    }
}



} // namespace lofibox::app

