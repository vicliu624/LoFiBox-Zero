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


def resolve(profile: Dict[str, Any], session: Dict[str, Any], track: Dict[str, Any]) -> Dict[str, Any]:
    token = session.get("access_token", "")
    return {"url": f"{base_url(profile)}/Audio/{track['id']}/stream?static=true&api_key={urllib.parse.quote(token)}", "headers": [], "seekable": True}
