// SPDX-License-Identifier: GPL-3.0-or-later

#include "application/library_open_action_service.h"

#include <algorithm>
#include <cstddef>

#include "application/library_query_service.h"

namespace lofibox::application {

LibraryOpenActionService::LibraryOpenActionService(app::LibraryController& controller) noexcept
    : controller_(controller)
{
}

app::LibraryOpenResult LibraryOpenActionService::openSelectedListItem(app::AppPage page, int selected) const
{
    switch (page) {
    case app::AppPage::MusicIndex:
        switch (selected) {
        case 0: return {app::LibraryOpenResult::Kind::PushPage, app::AppPage::Artists, 0};
        case 1:
            controller_.list_context_.albums = {};
            return {app::LibraryOpenResult::Kind::PushPage, app::AppPage::Albums, 0};
        case 2:
            controller_.setSongsContextAll();
            return {app::LibraryOpenResult::Kind::PushPage, app::AppPage::Songs, 0};
        case 3: return {app::LibraryOpenResult::Kind::PushPage, app::AppPage::Genres, 0};
        case 4: return {app::LibraryOpenResult::Kind::PushPage, app::AppPage::Composers, 0};
        case 5: return {app::LibraryOpenResult::Kind::PushPage, app::AppPage::Compilations, 0};
        default: return {};
        }
    case app::AppPage::Artists:
        if (selected >= 0 && selected < static_cast<int>(controller_.model().artists.size())) {
            controller_.list_context_.albums.mode = app::AlbumsMode::ByArtist;
            controller_.list_context_.albums.artist = controller_.model().artists[static_cast<std::size_t>(selected)];
            return {app::LibraryOpenResult::Kind::PushPage, app::AppPage::Albums, 0};
        }
        return {};
    case app::AppPage::Albums: {
        const auto albums = controller_.visibleAlbums();
        if (selected >= 0 && selected < static_cast<int>(albums.size())) {
            controller_.setSongsContextAlbum(albums[static_cast<std::size_t>(selected)]);
            return {app::LibraryOpenResult::Kind::PushPage, app::AppPage::Songs, 0};
        }
        return {};
    }
    case app::AppPage::Songs: {
        const auto ids = controller_.trackIdsForCurrentSongs();
        if (selected >= 0 && selected < static_cast<int>(ids.size())) {
            return {app::LibraryOpenResult::Kind::StartTrack, app::AppPage::NowPlaying, ids[static_cast<std::size_t>(selected)]};
        }
        return {};
    }
    case app::AppPage::Genres:
        if (selected >= 0 && selected < static_cast<int>(controller_.model().genres.size())) {
            const auto& genre = controller_.model().genres[static_cast<std::size_t>(selected)];
            controller_.setSongsContextFiltered(app::SongsMode::Genre, genre, LibraryQueryService::idsForGenre(controller_.model(), genre));
            return {app::LibraryOpenResult::Kind::PushPage, app::AppPage::Songs, 0};
        }
        return {};
    case app::AppPage::Composers:
        if (selected >= 0 && selected < static_cast<int>(controller_.model().composers.size())) {
            const auto& composer = controller_.model().composers[static_cast<std::size_t>(selected)];
            controller_.setSongsContextFiltered(app::SongsMode::Composer, composer, LibraryQueryService::idsForComposer(controller_.model(), composer));
            return {app::LibraryOpenResult::Kind::PushPage, app::AppPage::Songs, 0};
        }
        return {};
    case app::AppPage::Compilations:
        if (selected >= 0 && selected < static_cast<int>(controller_.model().compilations.size())) {
            const auto& compilation = controller_.model().compilations[static_cast<std::size_t>(selected)];
            controller_.setSongsContextFiltered(app::SongsMode::Compilation, compilation.album, compilation.track_ids);
            return {app::LibraryOpenResult::Kind::PushPage, app::AppPage::Songs, 0};
        }
        return {};
    case app::AppPage::Playlists: {
        const int index = std::clamp(selected, 0, 3);
        controller_.list_context_.playlist.kind = static_cast<app::PlaylistKind>(index);
        if (const auto rows = controller_.rowsForPage(app::AppPage::Playlists); rows && index < static_cast<int>(rows->size())) {
            controller_.list_context_.playlist.name = (*rows)[static_cast<std::size_t>(index)].first;
        }
        return {app::LibraryOpenResult::Kind::PushPage, app::AppPage::PlaylistDetail, 0};
    }
    case app::AppPage::PlaylistDetail: {
        const auto ids = controller_.playlistTrackIds();
        if (selected >= 0 && selected < static_cast<int>(ids.size())) {
            return {app::LibraryOpenResult::Kind::StartTrack, app::AppPage::NowPlaying, ids[static_cast<std::size_t>(selected)]};
        }
        return {};
    }
    default:
        return {};
    }
}

} // namespace lofibox::application
