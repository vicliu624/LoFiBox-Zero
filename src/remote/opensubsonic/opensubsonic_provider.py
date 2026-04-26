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


def resolve(profile: Dict[str, Any], track: Dict[str, Any]) -> Dict[str, Any]:
    request_params = params(profile)
    request_params["id"] = track["id"]
    return {"url": f"{base_url(profile)}/rest/stream.view?{urllib.parse.urlencode(request_params)}", "headers": [], "seekable": True}
