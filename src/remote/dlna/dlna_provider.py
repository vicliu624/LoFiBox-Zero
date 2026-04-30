# SPDX-License-Identifier: GPL-3.0-or-later

import html
import urllib.parse
import urllib.request
import xml.etree.ElementTree as ET
from typing import Any, Dict, List, Tuple

from remote.common import media_contract
from remote.tooling.http_json import base_url


DIDL_NS = {
    "didl": "urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/",
    "dc": "http://purl.org/dc/elements/1.1/",
    "upnp": "urn:schemas-upnp-org:metadata-1-0/upnp/",
}


VIDEO_EXTENSIONS = (
    ".mp4",
    ".m4v",
    ".mkv",
    ".avi",
    ".mov",
    ".webm",
    ".wmv",
    ".flv",
    ".mpeg",
    ".mpg",
)


def _read(url: str, body: bytes | None = None, headers: Dict[str, str] | None = None) -> bytes:
    request = urllib.request.Request(url, data=body, headers=headers or {}, method="POST" if body is not None else "GET")
    with urllib.request.urlopen(request, timeout=20) as response:
        return response.read()


def _service(profile: Dict[str, Any]) -> str:
    url = base_url(profile)
    description = ET.fromstring(_read(url))
    ns = {"d": "urn:schemas-upnp-org:device-1-0"}
    for service in description.findall(".//d:service", ns):
        service_type = service.findtext("d:serviceType", default="", namespaces=ns)
        if "ContentDirectory" not in service_type:
            continue
        control = service.findtext("d:controlURL", default="", namespaces=ns)
        return urllib.parse.urljoin(url, control)
    raise RuntimeError("ContentDirectory not found")


def _browse(profile: Dict[str, Any], object_id: str, flag: str = "BrowseDirectChildren", limit: int = 100) -> ET.Element:
    control_url = _service(profile)
    envelope = f"""<?xml version="1.0"?>
<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
  <s:Body>
    <u:Browse xmlns:u="urn:schemas-upnp-org:service:ContentDirectory:1">
      <ObjectID>{html.escape(object_id)}</ObjectID>
      <BrowseFlag>{flag}</BrowseFlag>
      <Filter>*</Filter>
      <StartingIndex>0</StartingIndex>
      <RequestedCount>{max(1, limit)}</RequestedCount>
      <SortCriteria></SortCriteria>
    </u:Browse>
  </s:Body>
</s:Envelope>""".encode("utf-8")
    response = ET.fromstring(
        _read(
            control_url,
            envelope,
            {
                "Content-Type": 'text/xml; charset="utf-8"',
                "SOAPACTION": '"urn:schemas-upnp-org:service:ContentDirectory:1#Browse"',
            },
        )
    )
    result_text = ""
    for element in response.iter():
        if element.tag.endswith("Result"):
            result_text = element.text or ""
            break
    return ET.fromstring(result_text.encode("utf-8"))


def _text(element: ET.Element, path: str) -> str:
    found = element.find(path, DIDL_NS)
    return found.text if found is not None and found.text else ""


def _looks_video_url(url: str) -> bool:
    path = urllib.parse.urlparse(url).path.casefold()
    return path.endswith(VIDEO_EXTENSIONS)


def _resource_protocol(item: ET.Element) -> Tuple[str, str]:
    resource = item.find("didl:res", DIDL_NS)
    if resource is None:
        return "", ""
    return resource.attrib.get("protocolInfo", "").casefold(), resource.text or ""


def _is_audio_item(item: ET.Element) -> bool:
    item_class = _text(item, "upnp:class").casefold()
    if "videoitem" in item_class or "movie" in item_class:
        return False
    if "audioitem" in item_class or "musictrack" in item_class:
        return True
    protocol, url = _resource_protocol(item)
    if "video/" in protocol or _looks_video_url(url):
        return False
    if "audio/" in protocol:
        return True
    return bool(url)


def _item_track(profile: Dict[str, Any], item: ET.Element) -> Dict[str, Any]:
    item_id = item.attrib.get("id", "")
    title = _text(item, "dc:title") or item_id
    artist = _text(item, "upnp:artist")
    album = _text(item, "upnp:album")
    resource = item.find("didl:res", DIDL_NS)
    url = resource.text if resource is not None and resource.text else item_id
    result = media_contract.track(profile, item_id, title, artist, album, "", 0)
    result["stream_url"] = url
    return result


def _item_node(profile: Dict[str, Any], item: ET.Element) -> Dict[str, Any]:
    track = _item_track(profile, item)
    return media_contract.node(
        profile,
        "tracks",
        track["id"],
        track["title"],
        track["artist"],
        True,
        False,
        track["artist"],
        track["album"],
        track["duration_seconds"],
        track["album_id"],
        track["artwork_key"],
        track["artwork_url"],
    )


def _container_node(profile: Dict[str, Any], container: ET.Element) -> Dict[str, Any]:
    item_id = container.attrib.get("id", "")
    title = _text(container, "dc:title") or item_id
    count = container.attrib.get("childCount", "")
    return media_contract.node(profile, "folder", item_id, title, count, False, True)


def _children(profile: Dict[str, Any], object_id: str, limit: int) -> List[Dict[str, Any]]:
    didl = _browse(profile, object_id, "BrowseDirectChildren", limit)
    result: List[Dict[str, Any]] = []
    for container in didl.findall("didl:container", DIDL_NS):
        result.append(_container_node(profile, container))
    for item in didl.findall("didl:item", DIDL_NS):
        if not _is_audio_item(item):
            continue
        result.append(_item_node(profile, item))
    return result[:limit]


def _metadata(profile: Dict[str, Any], object_id: str) -> Dict[str, Any]:
    didl = _browse(profile, object_id, "BrowseMetadata", 1)
    item = didl.find("didl:item", DIDL_NS)
    if item is None or not _is_audio_item(item):
        return {}
    return _item_track(profile, item)


def _library(profile: Dict[str, Any], limit: int) -> List[Dict[str, Any]]:
    tracks: List[Dict[str, Any]] = []
    stack = ["0"]
    seen = set()
    while stack and len(tracks) < limit:
        object_id = stack.pop()
        if object_id in seen:
            continue
        seen.add(object_id)
        for node in _children(profile, object_id, limit):
            if node.get("playable"):
                tracks.append(media_contract.track(
                    profile,
                    node.get("id", ""),
                    node.get("title", ""),
                    node.get("artist", ""),
                    node.get("album", ""),
                    node.get("album_id", ""),
                    node.get("duration_seconds", 0),
                    node.get("artwork_key", ""),
                    node.get("artwork_url", ""),
                ))
            elif node.get("browsable"):
                stack.append(node.get("id", ""))
            if len(tracks) >= limit:
                break
    return tracks


def probe(profile: Dict[str, Any]) -> Dict[str, Any]:
    try:
        _service(profile)
    except Exception as exc:
        return {
            "available": False,
            "server_name": profile.get("name", "DLNA / UPnP"),
            "user_id": "",
            "access_token": "",
            "message": type(exc).__name__,
        }
    return {
        "available": True,
        "server_name": profile.get("name", "DLNA / UPnP"),
        "user_id": "",
        "access_token": "",
        "message": "OK",
    }


def search(profile: Dict[str, Any], session: Dict[str, Any], query: str, limit: int) -> List[Dict[str, Any]]:
    del session
    needle = query.casefold()
    return [item for item in _library(profile, max(limit, 100)) if not needle or needle in item.get("title", "").casefold()][:limit]


def recent(profile: Dict[str, Any], session: Dict[str, Any], limit: int) -> List[Dict[str, Any]]:
    del session
    return _library(profile, limit)


def library_tracks(profile: Dict[str, Any], session: Dict[str, Any], limit: int) -> List[Dict[str, Any]]:
    del session
    return _library(profile, limit)


def browse(profile: Dict[str, Any], session: Dict[str, Any], parent: Dict[str, Any], limit: int) -> List[Dict[str, Any]]:
    del session
    object_id = parent.get("id") if parent.get("kind") == "folder" else "0"
    return _children(profile, object_id, limit)


def resolve(profile: Dict[str, Any], session: Dict[str, Any], track: Dict[str, Any]) -> Dict[str, Any]:
    del session
    metadata = _metadata(profile, track.get("id", "")) if track.get("id") else {}
    url = metadata.get("stream_url", "")
    return {
        "url": url,
        "headers": [],
        "seekable": True,
        "connected": bool(url),
        "connection_status": "READY" if url else "MISSING URL",
    }
