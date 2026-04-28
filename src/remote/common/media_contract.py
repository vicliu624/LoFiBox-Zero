# SPDX-License-Identifier: GPL-3.0-or-later

import urllib.parse
from typing import Any, Dict, Iterable, List

from remote.tooling.http_json import base_url


TRACK_FIELDS = (
    "id",
    "title",
    "artist",
    "album",
    "album_id",
    "duration_seconds",
    "source_id",
    "source_label",
    "artwork_key",
    "artwork_url",
    "lyrics_plain",
    "lyrics_synced",
    "lyrics_source",
    "fingerprint",
)

NODE_FIELDS = (
    "kind",
    "id",
    "title",
    "subtitle",
    "playable",
    "browsable",
    "artist",
    "album",
    "duration_seconds",
    "album_id",
    "source_id",
    "source_label",
    "artwork_key",
    "artwork_url",
    "lyrics_plain",
    "lyrics_synced",
    "lyrics_source",
    "fingerprint",
)


def _has_cjk(value: str) -> bool:
    return any(
        "\u3400" <= ch <= "\u4dbf"
        or "\u4e00" <= ch <= "\u9fff"
        or "\uf900" <= ch <= "\ufaff"
        for ch in value
    )


def _repair_mojibake(value: str) -> str:
    if not value or _has_cjk(value):
        return value
    try:
        raw = value.encode("latin1")
    except UnicodeEncodeError:
        return value
    for encoding in ("gb18030", "gbk", "big5", "utf-8"):
        try:
            candidate = raw.decode(encoding)
        except UnicodeDecodeError:
            continue
        if _has_cjk(candidate):
            return candidate
    return value


def text(value: Any) -> str:
    if value is None:
        return ""
    return _repair_mojibake(str(value))


def integer(value: Any) -> int:
    try:
        return int(value or 0)
    except (TypeError, ValueError):
        return 0


def source_id(profile: Dict[str, Any]) -> str:
    return text(profile.get("id") or profile.get("kind") or "remote")


def source_label(profile: Dict[str, Any]) -> str:
    return text(profile.get("name") or profile.get("kind") or "Remote")


def tokenless_item_artwork_url(profile: Dict[str, Any], item_id: str) -> str:
    if not item_id:
        return ""
    return f"{base_url(profile)}/Items/{urllib.parse.quote(item_id)}/Images/Primary?maxWidth=320"


def item_artwork_key(item: Dict[str, Any]) -> str:
    tags = item.get("ImageTags") or {}
    return text(tags.get("Primary") or item.get("PrimaryImageTag") or item.get("CoverArt") or item.get("coverArt") or "")


def track(
    profile: Dict[str, Any],
    item_id: Any,
    title: Any,
    artist: Any = "",
    album: Any = "",
    album_id: Any = "",
    duration_seconds: Any = 0,
    artwork_key: Any = "",
    artwork_url: Any = "",
    lyrics_plain: Any = "",
    lyrics_synced: Any = "",
    lyrics_source: Any = "",
    fingerprint: Any = "",
) -> Dict[str, Any]:
    return {
        "id": text(item_id),
        "title": text(title),
        "artist": text(artist),
        "album": text(album),
        "album_id": text(album_id),
        "duration_seconds": integer(duration_seconds),
        "source_id": source_id(profile),
        "source_label": source_label(profile),
        "artwork_key": text(artwork_key),
        "artwork_url": text(artwork_url),
        "lyrics_plain": text(lyrics_plain),
        "lyrics_synced": text(lyrics_synced),
        "lyrics_source": text(lyrics_source),
        "fingerprint": text(fingerprint),
    }


def node(
    profile: Dict[str, Any],
    kind: str,
    item_id: Any,
    title: Any,
    subtitle: Any = "",
    playable: bool = False,
    browsable: bool = True,
    artist: Any = "",
    album: Any = "",
    duration_seconds: Any = 0,
    album_id: Any = "",
    artwork_key: Any = "",
    artwork_url: Any = "",
    lyrics_plain: Any = "",
    lyrics_synced: Any = "",
    lyrics_source: Any = "",
    fingerprint: Any = "",
) -> Dict[str, Any]:
    return {
        "kind": text(kind),
        "id": text(item_id),
        "title": text(title),
        "subtitle": text(subtitle),
        "playable": bool(playable),
        "browsable": bool(browsable),
        "artist": text(artist),
        "album": text(album),
        "duration_seconds": integer(duration_seconds),
        "album_id": text(album_id),
        "source_id": source_id(profile),
        "source_label": source_label(profile),
        "artwork_key": text(artwork_key),
        "artwork_url": text(artwork_url),
        "lyrics_plain": text(lyrics_plain),
        "lyrics_synced": text(lyrics_synced),
        "lyrics_source": text(lyrics_source),
        "fingerprint": text(fingerprint),
    }


def normalize_track(profile: Dict[str, Any], value: Dict[str, Any]) -> Dict[str, Any]:
    normalized = track(
        profile,
        value.get("id", ""),
        value.get("title", ""),
        value.get("artist", ""),
        value.get("album", ""),
        value.get("album_id", ""),
        value.get("duration_seconds", 0),
        value.get("artwork_key", ""),
        value.get("artwork_url", ""),
        value.get("lyrics_plain", ""),
        value.get("lyrics_synced", ""),
        value.get("lyrics_source", ""),
        value.get("fingerprint", ""),
    )
    for key in ("source_id", "source_label"):
        if value.get(key):
            normalized[key] = text(value[key])
    return normalized


def normalize_node(profile: Dict[str, Any], value: Dict[str, Any]) -> Dict[str, Any]:
    normalized = node(
        profile,
        value.get("kind", "root"),
        value.get("id", ""),
        value.get("title", ""),
        value.get("subtitle", ""),
        bool(value.get("playable", False)),
        bool(value.get("browsable", True)),
        value.get("artist", ""),
        value.get("album", ""),
        value.get("duration_seconds", 0),
        value.get("album_id", ""),
        value.get("artwork_key", ""),
        value.get("artwork_url", ""),
        value.get("lyrics_plain", ""),
        value.get("lyrics_synced", ""),
        value.get("lyrics_source", ""),
        value.get("fingerprint", ""),
    )
    for key in ("source_id", "source_label"):
        if value.get(key):
            normalized[key] = text(value[key])
    return normalized


def tracks_response(profile: Dict[str, Any], tracks: Iterable[Dict[str, Any]]) -> Dict[str, List[Dict[str, Any]]]:
    return {"tracks": [normalize_track(profile, item) for item in tracks]}


def nodes_response(profile: Dict[str, Any], nodes: Iterable[Dict[str, Any]]) -> Dict[str, List[Dict[str, Any]]]:
    return {"nodes": [normalize_node(profile, item) for item in nodes]}


def stream_response(profile: Dict[str, Any], stream: Dict[str, Any]) -> Dict[str, Any]:
    result = dict(stream)
    result.setdefault("headers", [])
    result.setdefault("seekable", True)
    result.setdefault("source_id", source_id(profile))
    result.setdefault("source_label", source_label(profile))
    result.setdefault("bitrate_kbps", 0)
    result.setdefault("codec", "")
    result.setdefault("sample_rate_hz", 0)
    result.setdefault("channel_count", 0)
    result.setdefault("live", False)
    result.setdefault("connected", bool(result.get("url")))
    result.setdefault("connection_status", "READY" if result.get("url") else "MISSING URL")
    return result
