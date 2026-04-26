#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later

import hashlib
import json
import sys
import urllib.parse
import urllib.request
from typing import Any, Dict, List


def _json_request(url: str, headers: Dict[str, str] | None = None, method: str = "GET", body: bytes | None = None) -> Any:
    request = urllib.request.Request(url, data=body, headers=headers or {}, method=method)
    with urllib.request.urlopen(request, timeout=20) as response:
        return json.loads(response.read().decode("utf-8"))


def _musicbrainz_user_agent() -> str:
    return "LoFiBoxZero/0.1 (local development)"


def _emby_auth_header() -> str:
    return 'MediaBrowser Client="LoFiBox Zero", Device="LoFiBox Zero", DeviceId="lofibox-zero", Version="0.1.0"'


def _probe_jellyfin(profile: Dict[str, Any]) -> Dict[str, Any]:
    base = profile["base_url"].rstrip("/")
    token = profile.get("api_token", "")
    user_id = ""
    server_name = "Jellyfin"
    if token:
        info = _json_request(f"{base}/System/Info/Public")
        server_name = info.get("ServerName", server_name)
        return {"available": True, "server_name": server_name, "user_id": "", "access_token": token, "message": "OK"}

    body = json.dumps({"Username": profile.get("username", ""), "Pw": profile.get("password", "")}).encode("utf-8")
    auth = _json_request(
        f"{base}/Users/AuthenticateByName",
        headers={
            "Content-Type": "application/json",
            "X-Emby-Authorization": _emby_auth_header(),
        },
        method="POST",
        body=body,
    )
    token = auth.get("AccessToken", "")
    user = auth.get("User", {}) or {}
    user_id = user.get("Id", "")
    info = _json_request(f"{base}/System/Info/Public")
    server_name = info.get("ServerName", server_name)
    return {"available": bool(token), "server_name": server_name, "user_id": user_id, "access_token": token, "message": "OK" if token else "AUTH_FAILED"}


def _probe_emby(profile: Dict[str, Any]) -> Dict[str, Any]:
    base = profile["base_url"].rstrip("/")
    token = profile.get("api_token", "")
    user_id = ""
    server_name = "Emby"
    if token:
        info = _json_request(f"{base}/System/Info/Public")
        server_name = info.get("ServerName", server_name)
        return {"available": True, "server_name": server_name, "user_id": "", "access_token": token, "message": "OK"}

    body = json.dumps({"Username": profile.get("username", ""), "Pw": profile.get("password", "")}).encode("utf-8")
    auth = _json_request(
        f"{base}/Users/AuthenticateByName",
        headers={
            "Content-Type": "application/json",
            "X-Emby-Authorization": _emby_auth_header(),
        },
        method="POST",
        body=body,
    )
    token = auth.get("AccessToken", "")
    user = auth.get("User", {}) or {}
    user_id = user.get("Id", "")
    info = _json_request(f"{base}/System/Info/Public")
    server_name = info.get("ServerName", server_name)
    return {"available": bool(token), "server_name": server_name, "user_id": user_id, "access_token": token, "message": "OK" if token else "AUTH_FAILED"}


def _subsonic_params(profile: Dict[str, Any]) -> Dict[str, str]:
    username = profile.get("username", "")
    password = profile.get("password", "")
    salt = "lofiboxzero"
    token = hashlib.md5((password + salt).encode("utf-8")).hexdigest()
    return {
        "u": username,
        "t": token,
        "s": salt,
        "v": "1.16.1",
        "c": "LoFiBoxZero",
        "f": "json",
    }


def _probe_subsonic(profile: Dict[str, Any]) -> Dict[str, Any]:
    base = profile["base_url"].rstrip("/")
    params = urllib.parse.urlencode(_subsonic_params(profile))
    data = _json_request(f"{base}/rest/ping.view?{params}")
    status = (((data or {}).get("subsonic-response") or {}).get("status"))
    return {
        "available": status == "ok",
        "server_name": profile.get("name", "OpenSubsonic"),
        "user_id": profile.get("username", ""),
        "access_token": "",
        "message": "OK" if status == "ok" else "AUTH_FAILED",
    }


def _probe(profile: Dict[str, Any]) -> Dict[str, Any]:
    kind = profile["kind"]
    if kind == "jellyfin":
        return _probe_jellyfin(profile)
    if kind == "emby":
        return _probe_emby(profile)
    if kind == "opensubsonic":
        return _probe_subsonic(profile)
    raise ValueError(f"Unsupported kind: {kind}")


def _parse_jellyfin_items(items: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
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


def _search_jellyfin(profile: Dict[str, Any], session: Dict[str, Any], query: str, limit: int) -> List[Dict[str, Any]]:
    base = profile["base_url"].rstrip("/")
    user_id = session.get("user_id", "")
    token = session.get("access_token", "")
    params = urllib.parse.urlencode({
        "IncludeItemTypes": "Audio",
        "Recursive": "true",
        "SearchTerm": query,
        "Fields": "RunTimeTicks,AlbumPrimaryImageTag,PrimaryImageAspectRatio",
        "Limit": str(limit),
        "api_key": token,
    })
    data = _json_request(f"{base}/Users/{user_id}/Items?{params}")
    return _parse_jellyfin_items(data.get("Items", []))


def _recent_jellyfin(profile: Dict[str, Any], session: Dict[str, Any], limit: int) -> List[Dict[str, Any]]:
    base = profile["base_url"].rstrip("/")
    user_id = session.get("user_id", "")
    token = session.get("access_token", "")
    params = urllib.parse.urlencode({
        "IncludeItemTypes": "Audio",
        "Limit": str(limit),
        "api_key": token,
    })
    data = _json_request(f"{base}/Users/{user_id}/Items/Latest?{params}")
    return _parse_jellyfin_items(data)


def _search_emby(profile: Dict[str, Any], session: Dict[str, Any], query: str, limit: int) -> List[Dict[str, Any]]:
    base = profile["base_url"].rstrip("/")
    user_id = session.get("user_id", "")
    token = session.get("access_token", "")
    params = urllib.parse.urlencode({
        "IncludeItemTypes": "Audio",
        "Recursive": "true",
        "SearchTerm": query,
        "Fields": "RunTimeTicks,AlbumPrimaryImageTag,PrimaryImageAspectRatio",
        "Limit": str(limit),
        "api_key": token,
    })
    data = _json_request(f"{base}/Users/{user_id}/Items?{params}")
    return _parse_jellyfin_items(data.get("Items", []))


def _recent_emby(profile: Dict[str, Any], session: Dict[str, Any], limit: int) -> List[Dict[str, Any]]:
    base = profile["base_url"].rstrip("/")
    user_id = session.get("user_id", "")
    token = session.get("access_token", "")
    params = urllib.parse.urlencode({
        "IncludeItemTypes": "Audio",
        "Limit": str(limit),
        "api_key": token,
    })
    data = _json_request(f"{base}/Users/{user_id}/Items/Latest?{params}")
    return _parse_jellyfin_items(data)


def _search_subsonic(profile: Dict[str, Any], query: str, limit: int) -> List[Dict[str, Any]]:
    base = profile["base_url"].rstrip("/")
    params = _subsonic_params(profile)
    params.update({
        "query": query,
        "artistCount": "0",
        "albumCount": "0",
        "songCount": str(limit),
    })
    data = _json_request(f"{base}/rest/search3.view?{urllib.parse.urlencode(params)}")
    songs = (((data or {}).get("subsonic-response") or {}).get("searchResult3") or {}).get("song") or []
    return [{
        "id": item.get("id", ""),
        "title": item.get("title", ""),
        "artist": item.get("artist", ""),
        "album": item.get("album", ""),
        "album_id": item.get("parent", ""),
        "duration_seconds": int(item.get("duration", 0) or 0),
    } for item in songs]


def _recent_subsonic(profile: Dict[str, Any], limit: int) -> List[Dict[str, Any]]:
    base = profile["base_url"].rstrip("/")
    params = _subsonic_params(profile)
    params.update({
        "type": "newest",
        "size": str(limit),
    })
    data = _json_request(f"{base}/rest/getAlbumList2.view?{urllib.parse.urlencode(params)}")
    albums = (((data or {}).get("subsonic-response") or {}).get("albumList2") or {}).get("album") or []
    tracks: List[Dict[str, Any]] = []
    for album in albums:
        album_id = album.get("id")
        if not album_id:
            continue
        album_data = _json_request(f"{base}/rest/getAlbum.view?{urllib.parse.urlencode({**_subsonic_params(profile), 'id': album_id})}")
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


def _search(payload: Dict[str, Any]) -> Dict[str, Any]:
    profile = payload["profile"]
    session = payload["session"]
    query = payload["query"]
    limit = int(payload.get("limit", 20))
    kind = profile["kind"]
    if kind == "jellyfin":
        return {"tracks": _search_jellyfin(profile, session, query, limit)}
    if kind == "emby":
        return {"tracks": _search_emby(profile, session, query, limit)}
    if kind == "opensubsonic":
        return {"tracks": _search_subsonic(profile, query, limit)}
    raise ValueError(f"Unsupported kind: {kind}")


def _recent(payload: Dict[str, Any]) -> Dict[str, Any]:
    profile = payload["profile"]
    session = payload["session"]
    limit = int(payload.get("limit", 20))
    kind = profile["kind"]
    if kind == "jellyfin":
        return {"tracks": _recent_jellyfin(profile, session, limit)}
    if kind == "emby":
        return {"tracks": _recent_emby(profile, session, limit)}
    if kind == "opensubsonic":
        return {"tracks": _recent_subsonic(profile, limit)}
    raise ValueError(f"Unsupported kind: {kind}")


def _resolve(payload: Dict[str, Any]) -> Dict[str, Any]:
    profile = payload["profile"]
    session = payload["session"]
    track = payload["track"]
    base = profile["base_url"].rstrip("/")
    kind = profile["kind"]
    if kind == "jellyfin":
        token = session.get("access_token", "")
        return {"url": f"{base}/Audio/{track['id']}/stream?static=true&api_key={urllib.parse.quote(token)}", "headers": [], "seekable": True}
    if kind == "emby":
        token = session.get("access_token", "")
        user_id = session.get("user_id", "")
        return {"url": f"{base}/Audio/{track['id']}/stream.mp3?static=true&UserId={urllib.parse.quote(user_id)}&api_key={urllib.parse.quote(token)}", "headers": [], "seekable": True}
    if kind == "opensubsonic":
        params = _subsonic_params(profile)
        params["id"] = track["id"]
        return {"url": f"{base}/rest/stream.view?{urllib.parse.urlencode(params)}", "headers": [], "seekable": True}
    raise ValueError(f"Unsupported kind: {kind}")


def main(argv: List[str]) -> int:
    if len(argv) != 2:
        return 2
    with open(argv[1], "r", encoding="utf-8-sig") as handle:
        payload = json.load(handle)
    action = payload["action"]
    if action == "probe":
        result = _probe(payload["profile"])
    elif action == "search":
        result = _search(payload)
    elif action == "recent":
        result = _recent(payload)
    elif action == "resolve":
        result = _resolve(payload)
    else:
        raise ValueError(f"Unsupported action: {action}")
    sys.stdout.write(json.dumps(result, separators=(",", ":")))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
