# SPDX-License-Identifier: GPL-3.0-or-later

import configparser
import posixpath
import urllib.parse
import urllib.request
import xml.etree.ElementTree as ET
from typing import Any, Dict, Iterable, List

from remote.common import media_contract
from remote.tooling.http_json import base_url


AUDIO_EXTENSIONS = (".mp3", ".flac", ".m4a", ".aac", ".ogg", ".opus", ".wav")
VIDEO_EXTENSIONS = (".mp4", ".m4v", ".mkv", ".avi", ".mov", ".webm", ".wmv", ".flv", ".mpeg", ".mpg")


def _auth_headers(profile: Dict[str, Any]) -> Dict[str, str]:
    username = profile.get("username") or ""
    password = profile.get("password") or profile.get("api_token") or ""
    if not username and not password:
        return {}
    import base64

    token = base64.b64encode(f"{username}:{password}".encode("utf-8")).decode("ascii")
    return {"Authorization": f"Basic {token}"}


def _read_text(profile: Dict[str, Any]) -> str:
    url = base_url(profile)
    request = urllib.request.Request(url, headers=_auth_headers(profile))
    with urllib.request.urlopen(request, timeout=20) as response:
        return response.read().decode("utf-8-sig", errors="replace")


def _resolve_url(manifest_url: str, entry: str) -> str:
    entry = entry.strip()
    if not entry:
        return ""
    if "://" in entry:
        return entry
    return urllib.parse.urljoin(manifest_url, entry)


def _filename_title(url: str) -> str:
    parsed = urllib.parse.urlparse(url)
    name = posixpath.basename(urllib.parse.unquote(parsed.path.rstrip("/")))
    if not name:
        return url
    for ext in AUDIO_EXTENSIONS:
        if name.lower().endswith(ext):
            return name[: -len(ext)]
    return name


def _looks_video_url(url: str) -> bool:
    path = urllib.parse.urlparse(url).path.casefold()
    return path.endswith(VIDEO_EXTENSIONS)


def _track_from_url(profile: Dict[str, Any], url: str, title: str = "", duration_seconds: int = 0) -> Dict[str, Any]:
    title = title or _filename_title(url)
    return media_contract.track(profile, url, title, "", "", "", duration_seconds)


def _parse_m3u(profile: Dict[str, Any], text: str) -> List[Dict[str, Any]]:
    result: List[Dict[str, Any]] = []
    pending_title = ""
    pending_duration = 0
    manifest_url = base_url(profile)
    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line:
            continue
        if line.startswith("#EXTINF:"):
            payload = line[len("#EXTINF:") :]
            duration_text, _, title = payload.partition(",")
            try:
                pending_duration = max(0, int(float(duration_text.strip())))
            except ValueError:
                pending_duration = 0
            pending_title = title.strip()
            continue
        if line.startswith("#"):
            continue
        url = _resolve_url(manifest_url, line)
        if url and not _looks_video_url(url):
            result.append(_track_from_url(profile, url, pending_title, pending_duration))
        pending_title = ""
        pending_duration = 0
    return result


def _parse_pls(profile: Dict[str, Any], text: str) -> List[Dict[str, Any]]:
    parser = configparser.ConfigParser()
    parser.read_string(text)
    if not parser.has_section("playlist"):
        return []
    manifest_url = base_url(profile)
    result: List[Dict[str, Any]] = []
    index = 1
    while True:
        file_key = f"File{index}"
        if not parser.has_option("playlist", file_key):
            break
        url = _resolve_url(manifest_url, parser.get("playlist", file_key))
        title = parser.get("playlist", f"Title{index}", fallback="")
        length = parser.get("playlist", f"Length{index}", fallback="0")
        try:
            duration_seconds = max(0, int(length))
        except ValueError:
            duration_seconds = 0
        if url and not _looks_video_url(url):
            result.append(_track_from_url(profile, url, title, duration_seconds))
        index += 1
    return result


def _parse_xspf(profile: Dict[str, Any], text: str) -> List[Dict[str, Any]]:
    manifest_url = base_url(profile)
    root = ET.fromstring(text)
    ns = {"xspf": "http://xspf.org/ns/0/"}
    tracks = root.findall(".//xspf:track", ns) or root.findall(".//track")
    result: List[Dict[str, Any]] = []
    for item in tracks:
        location = item.findtext("xspf:location", default="", namespaces=ns) or item.findtext("location", default="")
        title = item.findtext("xspf:title", default="", namespaces=ns) or item.findtext("title", default="")
        duration_ms = item.findtext("xspf:duration", default="", namespaces=ns) or item.findtext("duration", default="")
        try:
            duration_seconds = max(0, int(int(duration_ms) / 1000))
        except ValueError:
            duration_seconds = 0
        url = _resolve_url(manifest_url, location)
        if url and not _looks_video_url(url):
            result.append(_track_from_url(profile, url, title, duration_seconds))
    return result


def _format(profile: Dict[str, Any]) -> str:
    path = urllib.parse.urlparse(base_url(profile)).path.lower()
    if path.endswith(".pls"):
        return "pls"
    if path.endswith(".xspf"):
        return "xspf"
    return "m3u"


def _tracks(profile: Dict[str, Any]) -> List[Dict[str, Any]]:
    text = _read_text(profile)
    fmt = _format(profile)
    if fmt == "pls":
        return _parse_pls(profile, text)
    if fmt == "xspf":
        return _parse_xspf(profile, text)
    return _parse_m3u(profile, text)


def probe(profile: Dict[str, Any]) -> Dict[str, Any]:
    try:
        count = len(_tracks(profile))
    except Exception as exc:
        return {
            "available": False,
            "server_name": profile.get("name", "Playlist"),
            "user_id": profile.get("username", ""),
            "access_token": "",
            "message": type(exc).__name__,
        }
    return {
        "available": count > 0,
        "server_name": profile.get("name", "Playlist"),
        "user_id": profile.get("username", ""),
        "access_token": "",
        "message": f"{count} TRACKS",
    }


def search(profile: Dict[str, Any], session: Dict[str, Any], query: str, limit: int) -> List[Dict[str, Any]]:
    del session
    needle = query.casefold()
    return [item for item in _tracks(profile) if not needle or needle in item.get("title", "").casefold()][:limit]


def recent(profile: Dict[str, Any], session: Dict[str, Any], limit: int) -> List[Dict[str, Any]]:
    del session
    return _tracks(profile)[:limit]


def library_tracks(profile: Dict[str, Any], session: Dict[str, Any], limit: int) -> List[Dict[str, Any]]:
    del session
    return _tracks(profile)[:limit]


def browse(profile: Dict[str, Any], session: Dict[str, Any], parent: Dict[str, Any], limit: int) -> List[Dict[str, Any]]:
    del session
    kind = parent.get("kind", "root")
    if kind not in ("root", "", "tracks"):
        return []
    return [
        media_contract.node(
            profile,
            "tracks",
            item["id"],
            item["title"],
            item.get("artist", ""),
            True,
            False,
            item.get("artist", ""),
            item.get("album", ""),
            item.get("duration_seconds", 0),
        )
        for item in _tracks(profile)[:limit]
    ]


def resolve(profile: Dict[str, Any], session: Dict[str, Any], track: Dict[str, Any]) -> Dict[str, Any]:
    del session
    url = track.get("id", "")
    return {
        "url": url,
        "headers": [],
        "seekable": True,
        "connected": bool(url),
        "connection_status": "READY" if url else "MISSING URL",
    }
