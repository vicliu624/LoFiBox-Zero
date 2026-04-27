# SPDX-License-Identifier: GPL-3.0-or-later

import urllib.parse
from typing import Any, Dict, List

from remote.jellyfin import jellyfin_provider
from remote.tooling.http_json import base_url, json_request


def probe(profile: Dict[str, Any]) -> Dict[str, Any]:
    result = jellyfin_provider.probe(profile)
    if result.get("server_name") == "Jellyfin":
        result["server_name"] = "Emby"
    return result


def playable_item_query(
    profile: Dict[str, Any],
    session: Dict[str, Any],
    limit: int,
    query: str = "",
    sort_by: str = "SortName",
    sort_order: str = "Ascending",
) -> List[Dict[str, Any]]:
    user_id = session.get("user_id", "")
    token = session.get("access_token", "")
    params_dict = {
        "Recursive": "true",
        "Filters": "IsNotFolder",
        "MediaTypes": "Video",
        "Fields": "RunTimeTicks,MediaSources,Path,SeriesName",
        "Limit": str(limit),
        "SortBy": sort_by,
        "SortOrder": sort_order,
        "api_key": token,
    }
    if query:
        params_dict["SearchTerm"] = query
    data = json_request(f"{base_url(profile)}{jellyfin_provider.items_path(user_id)}?{urllib.parse.urlencode(params_dict)}")
    return jellyfin_provider.parse_items(data.get("Items", []))


def search(profile: Dict[str, Any], session: Dict[str, Any], query: str, limit: int) -> List[Dict[str, Any]]:
    tracks = jellyfin_provider.search(profile, session, query, limit)
    return tracks if tracks else playable_item_query(profile, session, limit, query=query)


def recent(profile: Dict[str, Any], session: Dict[str, Any], limit: int) -> List[Dict[str, Any]]:
    tracks = jellyfin_provider.recent(profile, session, limit)
    return tracks if tracks else playable_item_query(profile, session, limit, sort_by="DateCreated", sort_order="Descending")


def library_tracks(profile: Dict[str, Any], session: Dict[str, Any], limit: int) -> List[Dict[str, Any]]:
    tracks = jellyfin_provider.library_tracks(profile, session, limit)
    return tracks if tracks else playable_item_query(profile, session, limit)


def browse(profile: Dict[str, Any], session: Dict[str, Any], parent: Dict[str, Any], limit: int) -> List[Dict[str, Any]]:
    nodes = jellyfin_provider.browse(profile, session, parent, limit)
    if nodes:
        return nodes
    kind = parent.get("kind", "root")
    if kind in ("tracks", "recently-added", "recently-played", "favorites"):
        return [
            jellyfin_provider.node("tracks", item["id"], item["title"], item.get("artist", ""), playable=True, browsable=False)
            for item in playable_item_query(profile, session, limit)
        ]
    return nodes


def resolve(profile: Dict[str, Any], session: Dict[str, Any], track: Dict[str, Any]) -> Dict[str, Any]:
    token = session.get("access_token", "")
    user_id = session.get("user_id", "")
    return {
        "url": f"{base_url(profile)}/Audio/{track['id']}/stream.mp3?static=true&UserId={urllib.parse.quote(user_id)}&api_key={urllib.parse.quote(token)}",
        "headers": [],
        "seekable": True,
    }
