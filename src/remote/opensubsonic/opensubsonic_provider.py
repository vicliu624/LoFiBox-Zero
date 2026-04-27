# SPDX-License-Identifier: GPL-3.0-or-later

import hashlib
import urllib.parse
from typing import Any, Dict, List

from remote.tooling.http_json import base_url, json_request


def params(profile: Dict[str, Any]) -> Dict[str, str]:
    username = profile.get("username", "")
    password = profile.get("password", "")
    salt = "lofiboxzero"
    token = hashlib.md5((password + salt).encode("utf-8")).hexdigest()
    return {"u": username, "t": token, "s": salt, "v": "1.16.1", "c": "LoFiBoxZero", "f": "json"}


def probe(profile: Dict[str, Any]) -> Dict[str, Any]:
    data = json_request(f"{base_url(profile)}/rest/ping.view?{urllib.parse.urlencode(params(profile))}")
    status = (((data or {}).get("subsonic-response") or {}).get("status"))
    return {
        "available": status == "ok",
        "server_name": profile.get("name", "OpenSubsonic"),
        "user_id": profile.get("username", ""),
        "access_token": "",
        "message": "OK" if status == "ok" else "AUTH_FAILED",
    }


def search(profile: Dict[str, Any], query: str, limit: int) -> List[Dict[str, Any]]:
    request_params = params(profile)
    request_params.update({"query": query, "artistCount": "0", "albumCount": "0", "songCount": str(limit)})
    data = json_request(f"{base_url(profile)}/rest/search3.view?{urllib.parse.urlencode(request_params)}")
    songs = (((data or {}).get("subsonic-response") or {}).get("searchResult3") or {}).get("song") or []
    return [{
        "id": item.get("id", ""),
        "title": item.get("title", ""),
        "artist": item.get("artist", ""),
        "album": item.get("album", ""),
        "album_id": item.get("parent", ""),
        "duration_seconds": int(item.get("duration", 0) or 0),
    } for item in songs]


def node(kind: str, item_id: str, title: str, subtitle: str = "", playable: bool = False, browsable: bool = True) -> Dict[str, Any]:
    return {
        "kind": kind,
        "id": item_id,
        "title": title,
        "subtitle": subtitle,
        "playable": playable,
        "browsable": browsable,
    }


def recent(profile: Dict[str, Any], limit: int) -> List[Dict[str, Any]]:
    request_params = params(profile)
    request_params.update({"type": "newest", "size": str(limit)})
    data = json_request(f"{base_url(profile)}/rest/getAlbumList2.view?{urllib.parse.urlencode(request_params)}")
    albums = (((data or {}).get("subsonic-response") or {}).get("albumList2") or {}).get("album") or []
    tracks: List[Dict[str, Any]] = []
    for album in albums:
        album_id = album.get("id")
        if not album_id:
            continue
        album_data = json_request(f"{base_url(profile)}/rest/getAlbum.view?{urllib.parse.urlencode({**params(profile), 'id': album_id})}")
        songs = (((album_data or {}).get("subsonic-response") or {}).get("album") or {}).get("song") or []
        for item in songs:
            tracks.append({
                "id": item.get("id", ""),
                "title": item.get("title", ""),
                "artist": item.get("artist", ""),
                "album": item.get("album", ""),
                "album_id": item.get("parent", ""),
                "duration_seconds": int(item.get("duration", 0) or 0),
            })
            if len(tracks) >= limit:
                return tracks
    return tracks


def library_tracks(profile: Dict[str, Any], limit: int) -> List[Dict[str, Any]]:
    return recent(profile, limit)


def browse(profile: Dict[str, Any], parent: Dict[str, Any], limit: int) -> List[Dict[str, Any]]:
    kind = parent.get("kind", "root")
    if kind in ("root", ""):
        return [
            node("artists", "artists", "ARTISTS", "Server artists"),
            node("albums", "albums", "ALBUMS", "Server albums"),
            node("tracks", "tracks", "TRACKS", "All songs"),
            node("playlists", "playlists", "PLAYLISTS", "Server playlists"),
            node("recently-added", "recently-added", "RECENT ADD", "Recently added"),
            node("recently-played", "recently-played", "RECENT PLAY", "Recently played"),
            node("favorites", "favorites", "FAVORITES", "Starred songs"),
        ]
    if kind == "artists":
        data = json_request(f"{base_url(profile)}/rest/getArtists.view?{urllib.parse.urlencode(params(profile))}")
        indexes = (((data or {}).get("subsonic-response") or {}).get("artists") or {}).get("index") or []
        result: List[Dict[str, Any]] = []
        for index in indexes:
            for artist in index.get("artist") or []:
                result.append(node("artist", artist.get("id", ""), artist.get("name", "")))
                if len(result) >= limit:
                    return result
        return result
    if kind == "albums":
        request_params = params(profile)
        request_params.update({"type": "alphabeticalByName", "size": str(limit)})
        data = json_request(f"{base_url(profile)}/rest/getAlbumList2.view?{urllib.parse.urlencode(request_params)}")
        albums = (((data or {}).get("subsonic-response") or {}).get("albumList2") or {}).get("album") or []
        return [node("album", album.get("id", ""), album.get("name", ""), album.get("artist", "")) for album in albums]
    if kind == "playlists":
        data = json_request(f"{base_url(profile)}/rest/getPlaylists.view?{urllib.parse.urlencode(params(profile))}")
        playlists = (((data or {}).get("subsonic-response") or {}).get("playlists") or {}).get("playlist") or []
        return [node("playlist", item.get("id", ""), item.get("name", ""), "", False, True) for item in playlists[:limit]]
    if kind == "favorites":
        data = json_request(f"{base_url(profile)}/rest/getStarred2.view?{urllib.parse.urlencode(params(profile))}")
        songs = (((data or {}).get("subsonic-response") or {}).get("starred2") or {}).get("song") or []
        return [node("tracks", item.get("id", ""), item.get("title", ""), item.get("artist", ""), True, False) for item in songs[:limit]]
    if kind == "recently-added":
        return [node("tracks", item["id"], item["title"], item.get("artist", ""), True, False) for item in recent(profile, limit)]
    if kind == "recently-played":
        return [node("tracks", item["id"], item["title"], item.get("artist", ""), True, False) for item in recent(profile, limit)]
    if kind == "album":
        data = json_request(f"{base_url(profile)}/rest/getAlbum.view?{urllib.parse.urlencode({**params(profile), 'id': parent.get('id', '')})}")
        songs = (((data or {}).get("subsonic-response") or {}).get("album") or {}).get("song") or []
        return [node("tracks", item.get("id", ""), item.get("title", ""), item.get("artist", ""), True, False) for item in songs[:limit]]
    if kind == "playlist":
        data = json_request(f"{base_url(profile)}/rest/getPlaylist.view?{urllib.parse.urlencode({**params(profile), 'id': parent.get('id', '')})}")
        songs = (((data or {}).get("subsonic-response") or {}).get("playlist") or {}).get("entry") or []
        return [node("tracks", item.get("id", ""), item.get("title", ""), item.get("artist", ""), True, False) for item in songs[:limit]]
    if kind == "artist":
        data = json_request(f"{base_url(profile)}/rest/getArtist.view?{urllib.parse.urlencode({**params(profile), 'id': parent.get('id', '')})}")
        albums = (((data or {}).get("subsonic-response") or {}).get("artist") or {}).get("album") or []
        return [node("album", album.get("id", ""), album.get("name", ""), album.get("artist", "")) for album in albums[:limit]]
    if kind == "tracks":
        return [node("tracks", item["id"], item["title"], item.get("artist", ""), True, False) for item in library_tracks(profile, limit)]
    return []


def resolve(profile: Dict[str, Any], track: Dict[str, Any]) -> Dict[str, Any]:
    request_params = params(profile)
    request_params["id"] = track["id"]
    return {"url": f"{base_url(profile)}/rest/stream.view?{urllib.parse.urlencode(request_params)}", "headers": [], "seekable": True}
