// SPDX-License-Identifier: GPL-3.0-or-later

#include "remote/common/remote_catalog_model.h"

namespace lofibox::remote {

std::vector<app::RemoteCatalogNode> RemoteCatalogMap::rootNodes()
{
    return {
        {app::RemoteCatalogNodeKind::Artists, "artists", "ARTISTS", "Server artists", false, true},
        {app::RemoteCatalogNodeKind::Albums, "albums", "ALBUMS", "Server albums", false, true},
        {app::RemoteCatalogNodeKind::Tracks, "tracks", "TRACKS", "All remote tracks", false, true},
        {app::RemoteCatalogNodeKind::Genres, "genres", "GENRES", "Server genres", false, true},
        {app::RemoteCatalogNodeKind::Playlists, "playlists", "PLAYLISTS", "Server playlists", false, true},
        {app::RemoteCatalogNodeKind::Folders, "folders", "FOLDERS", "Folder view", false, true},
        {app::RemoteCatalogNodeKind::Favorites, "favorites", "FAVORITES", "Favorite media", false, true},
        {app::RemoteCatalogNodeKind::RecentlyAdded, "recently-added", "RECENT ADD", "Recently added", false, true},
        {app::RemoteCatalogNodeKind::RecentlyPlayed, "recently-played", "RECENT PLAY", "Recently played", false, true},
        {app::RemoteCatalogNodeKind::Stations, "stations", "STATIONS", "Radio and live entries", false, true},
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
