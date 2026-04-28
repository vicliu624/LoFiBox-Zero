// SPDX-License-Identifier: GPL-3.0-or-later

#include "remote/common/remote_catalog_model.h"

namespace lofibox::remote {

std::vector<app::RemoteCatalogNode> RemoteCatalogMap::rootNodes()
{
    return {
        {app::RemoteCatalogNodeKind::Artists, "artists", "ARTISTS", "0", false, true},
        {app::RemoteCatalogNodeKind::Albums, "albums", "ALBUMS", "0", false, true},
        {app::RemoteCatalogNodeKind::Tracks, "tracks", "TRACKS", "0", false, true},
        {app::RemoteCatalogNodeKind::Genres, "genres", "GENRES", "0", false, true},
        {app::RemoteCatalogNodeKind::Playlists, "playlists", "PLAYLISTS", "0", false, true},
        {app::RemoteCatalogNodeKind::Folders, "folders", "FOLDERS", "0", false, true},
        {app::RemoteCatalogNodeKind::Favorites, "favorites", "FAVORITES", "0", false, true},
        {app::RemoteCatalogNodeKind::RecentlyAdded, "recently-added", "RECENT ADD", "0", false, true},
        {app::RemoteCatalogNodeKind::RecentlyPlayed, "recently-played", "RECENT PLAY", "0", false, true},
        {app::RemoteCatalogNodeKind::Stations, "stations", "STATIONS", "0", false, true},
    };
}

std::vector<app::RemoteCatalogNodeKind> RemoteCatalogMap::supportedHierarchy()
{
    return {
        app::RemoteCatalogNodeKind::Artists,
        app::RemoteCatalogNodeKind::Artist,
        app::RemoteCatalogNodeKind::Albums,
        app::RemoteCatalogNodeKind::Album,
        app::RemoteCatalogNodeKind::Tracks,
        app::RemoteCatalogNodeKind::Genres,
        app::RemoteCatalogNodeKind::Genre,
        app::RemoteCatalogNodeKind::Playlists,
        app::RemoteCatalogNodeKind::Playlist,
        app::RemoteCatalogNodeKind::Folders,
        app::RemoteCatalogNodeKind::Folder,
        app::RemoteCatalogNodeKind::Favorites,
        app::RemoteCatalogNodeKind::RecentlyAdded,
        app::RemoteCatalogNodeKind::RecentlyPlayed,
        app::RemoteCatalogNodeKind::Stations,
    };
}

std::string RemoteCatalogMap::explainNode(app::RemoteCatalogNodeKind kind)
{
    switch (kind) {
    case app::RemoteCatalogNodeKind::Artists: return "Browse remote artists.";
    case app::RemoteCatalogNodeKind::Albums: return "Browse remote albums.";
    case app::RemoteCatalogNodeKind::Tracks: return "Browse remote tracks.";
    case app::RemoteCatalogNodeKind::Genres: return "Browse remote genres.";
    case app::RemoteCatalogNodeKind::Playlists: return "Browse remote playlists.";
    case app::RemoteCatalogNodeKind::Folders: return "Browse remote folder paths.";
    case app::RemoteCatalogNodeKind::Favorites: return "Browse server favorites.";
    case app::RemoteCatalogNodeKind::RecentlyAdded: return "Browse recently added remote items.";
    case app::RemoteCatalogNodeKind::RecentlyPlayed: return "Browse recently played remote items.";
    case app::RemoteCatalogNodeKind::Stations: return "Browse radio and live stations.";
    default: return "Browse remote media.";
    }
}

} // namespace lofibox::remote
