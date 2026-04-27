# SPDX-License-Identifier: GPL-3.0-or-later

import json
import urllib.parse
from typing import Any, Dict, List

from remote.tooling.http_json import base_url, json_request


def auth_header() -> str:
    return 'MediaBrowser Client="LoFiBox Zero", Device="LoFiBox Zero", DeviceId="lofibox-zero", Version="0.1.0"'


def probe(profile: Dict[str, Any]) -> Dict[str, Any]:
    base = base_url(profile)
    token = profile.get("api_token", "")
    server_name = "Jellyfin"
    if token:
        info = json_request(f"{base}/System/Info/Public")
        server_name = info.get("ServerName", server_name)
        return {"available": True, "server_name": server_name, "user_id": "", "access_token": token, "message": "OK"}

    body = json.dumps({"Username": profile.get("username", ""), "Pw": profile.get("password", "")}).encode("utf-8")
    auth = json_request(
        f"{base}/Users/AuthenticateByName",
        headers={"Content-Type": "application/json", "X-Emby-Authorization": auth_header()},
        method="POST",
        body=body,
    )
    token = auth.get("AccessToken", "")
    user = auth.get("User", {}) or {}
    info = json_request(f"{base}/System/Info/Public")
    server_name = info.get("ServerName", server_name)
    return {
        "available": bool(token),
        "server_name": server_name,
        "user_id": user.get("Id", ""),
        "access_token": token,
        "message": "OK" if token else "AUTH_FAILED",
    }


def parse_items(items: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
    tracks = []
    for item in items:
        artists = item.get("Artists") or []
        tracks.append({
            "id": item.get("Id", ""),
            "title": item.get("Name", ""),
            "artist": artists[0] if artists else "",
            "album": item.get("Album", ""),
            "album_id": item.get("AlbumId", ""),
            "duration_seconds": int((item.get("RunTimeTicks") or 0) / 10000000),
        })
    return tracks


def node(kind: str, item_id: str, title: str, subtitle: str = "", playable: bool = False, browsable: bool = True) -> Dict[str, Any]:
    return {
        "kind": kind,
        "id": item_id,
        "title": title,
        "subtitle": subtitle,
        "playable": playable,
        "browsable": browsable,
    }


def parse_nodes(items: List[Dict[str, Any]], kind: str, playable: bool = False) -> List[Dict[str, Any]]:
    nodes: List[Dict[str, Any]] = []
    for item in items:
        title = item.get("Name") or item.get("Title") or item.get("Id", "")
        item_id = item.get("Id", "")
        if title or item_id:
            nodes.append(node(kind, item_id, title, item.get("Album", ""), playable=playable, browsable=not playable))
    return nodes


def items_path(user_id: str) -> str:
    return f"/Users/{urllib.parse.quote(user_id)}/Items" if user_id else "/Items"


def latest_path(user_id: str) -> str:
    return f"/Users/{urllib.parse.quote(user_id)}/Items/Latest" if user_id else "/Items/Latest"


def item_query(
    profile: Dict[str, Any],
    session: Dict[str, Any],
    limit: int,
    query: str = "",
    sort_by: str = "SortName",
    sort_order: str = "Ascending",
) -> List[Dict[str, Any]]:
    base = base_url(profile)
    user_id = session.get("user_id", "")
    token = session.get("access_token", "")
    params_dict = {
        "IncludeItemTypes": "Audio",
        "Recursive": "true",
        "Fields": "RunTimeTicks,AlbumPrimaryImageTag,PrimaryImageAspectRatio,MediaSources,Path",
        "Limit": str(limit),
        "SortBy": sort_by,
        "SortOrder": sort_order,
        "api_key": token,
    }
    if query:
        params_dict["SearchTerm"] = query
    data = json_request(f"{base}{items_path(user_id)}?{urllib.parse.urlencode(params_dict)}")
    return parse_items(data.get("Items", []))


def search(profile: Dict[str, Any], session: Dict[str, Any], query: str, limit: int) -> List[Dict[str, Any]]:
    return item_query(profile, session, limit, query=query)


def recent(profile: Dict[str, Any], session: Dict[str, Any], limit: int) -> List[Dict[str, Any]]:
    base = base_url(profile)
    user_id = session.get("user_id", "")
    token = session.get("access_token", "")
    params = urllib.parse.urlencode({"IncludeItemTypes": "Audio", "Limit": str(limit), "api_key": token})
    return parse_items(json_request(f"{base}{latest_path(user_id)}?{params}"))


def library_tracks(profile: Dict[str, Any], session: Dict[str, Any], limit: int) -> List[Dict[str, Any]]:
    return item_query(profile, session, limit)


def browse(profile: Dict[str, Any], session: Dict[str, Any], parent: Dict[str, Any], limit: int) -> List[Dict[str, Any]]:
    base = base_url(profile)
    user_id = session.get("user_id", "")
    token = session.get("access_token", "")
    kind = parent.get("kind", "root")
    parent_id = parent.get("id", "")

    if kind in ("root", ""):
        return [
            node("artists", "artists", "ARTISTS", "Server artists"),
            node("albums", "albums", "ALBUMS", "Server albums"),
            node("tracks", "tracks", "TRACKS", "All audio tracks"),
            node("genres", "genres", "GENRES", "Server genres"),
            node("playlists", "playlists", "PLAYLISTS", "Server playlists"),
            node("folders", "folders", "FOLDERS", "Folder view"),
            node("favorites", "favorites", "FAVORITES", "Favorite tracks"),
            node("recently-added", "recently-added", "RECENT ADD", "Recently added"),
            node("recently-played", "recently-played", "RECENT PLAY", "Recently played"),
        ]

    common = {"Limit": str(limit), "api_key": token}
    if kind == "artists":
        params = urllib.parse.urlencode({**common, "IncludeItemTypes": "MusicArtist", "Recursive": "true", "SortBy": "SortName"})
        return parse_nodes(json_request(f"{base}{items_path(user_id)}?{params}").get("Items", []), "artist")
    if kind == "albums":
        params = urllib.parse.urlencode({**common, "IncludeItemTypes": "MusicAlbum", "Recursive": "true", "SortBy": "SortName"})
        return parse_nodes(json_request(f"{base}{items_path(user_id)}?{params}").get("Items", []), "album")
    if kind == "genres":
        params = urllib.parse.urlencode({**common, "IncludeItemTypes": "MusicGenre", "Recursive": "true", "SortBy": "SortName"})
        return parse_nodes(json_request(f"{base}{items_path(user_id)}?{params}").get("Items", []), "genre")
    if kind == "playlists":
        params = urllib.parse.urlencode({**common, "IncludeItemTypes": "Playlist", "Recursive": "true", "SortBy": "SortName"})
        return parse_nodes(json_request(f"{base}{items_path(user_id)}?{params}").get("Items", []), "playlist")
    if kind == "folders":
        params = urllib.parse.urlencode({**common, "Recursive": "false", "SortBy": "SortName"})
        return parse_nodes(json_request(f"{base}{items_path(user_id)}?{params}").get("Items", []), "folder")
    if kind == "favorites":
        params = urllib.parse.urlencode({**common, "IncludeItemTypes": "Audio", "Recursive": "true", "Filters": "IsFavorite", "Fields": "RunTimeTicks,MediaSources"})
        return parse_nodes(json_request(f"{base}{items_path(user_id)}?{params}").get("Items", []), "tracks", playable=True)
    if kind == "recently-added":
        return [node("tracks", item["id"], item["title"], item.get("artist", ""), playable=True, browsable=False) for item in recent(profile, session, limit)]
    if kind == "recently-played":
        params = urllib.parse.urlencode({**common, "IncludeItemTypes": "Audio", "Recursive": "true", "SortBy": "DatePlayed", "SortOrder": "Descending", "Fields": "RunTimeTicks,MediaSources"})
        return parse_nodes(json_request(f"{base}{items_path(user_id)}?{params}").get("Items", []), "tracks", playable=True)
    if kind in ("artist", "album", "genre", "playlist", "folder"):
        params_dict = {**common, "IncludeItemTypes": "Audio", "Recursive": "true", "Fields": "RunTimeTicks,MediaSources"}
        if kind == "album":
            params_dict["ParentId"] = parent_id
        elif kind == "playlist":
            params_dict["ParentId"] = parent_id
        elif kind == "folder":
            params_dict["ParentId"] = parent_id
            params_dict["Recursive"] = "false"
        else:
            params_dict["SearchTerm"] = parent.get("title", "")
        params = urllib.parse.urlencode(params_dict)
        return parse_nodes(json_request(f"{base}{items_path(user_id)}?{params}").get("Items", []), "tracks", playable=True)
    if kind == "tracks":
        return [node("tracks", item["id"], item["title"], item.get("artist", ""), playable=True, browsable=False) for item in library_tracks(profile, session, limit)]
    return []


def resolve(profile: Dict[str, Any], session: Dict[str, Any], track: Dict[str, Any]) -> Dict[str, Any]:
    token = session.get("access_token", "")
    return {"url": f"{base_url(profile)}/Audio/{track['id']}/stream?static=true&api_key={urllib.parse.quote(token)}", "headers": [], "seekable": True}
