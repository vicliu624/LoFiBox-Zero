# SPDX-License-Identifier: GPL-3.0-or-later

import ftplib
import json
import posixpath
import shlex
import subprocess
import urllib.parse
import urllib.request
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from typing import Any, Dict, Iterable, List, Optional

from remote.common import media_contract
from remote.tooling.http_json import base_url


AUDIO_EXTENSIONS = (".mp3", ".flac", ".m4a", ".aac", ".ogg", ".opus", ".wav")


@dataclass
class ShareEntry:
    url: str
    name: str
    is_dir: bool
    size: int = 0


def _kind(profile: Dict[str, Any]) -> str:
    return str(profile.get("kind") or "").casefold()


def _is_audio(path: str) -> bool:
    return path.casefold().endswith(AUDIO_EXTENSIONS)


def _title(path: str) -> str:
    name = posixpath.basename(urllib.parse.unquote(path.rstrip("/")))
    for ext in AUDIO_EXTENSIONS:
        if name.casefold().endswith(ext):
            return name[: -len(ext)]
    return name or path


def _track(profile: Dict[str, Any], url: str) -> Dict[str, Any]:
    parts = [part for part in urllib.parse.unquote(urllib.parse.urlparse(url).path).split("/") if part]
    album = parts[-2] if len(parts) >= 2 else ""
    artist = parts[-3] if len(parts) >= 3 else ""
    return media_contract.track(profile, url, _title(url), artist, album, "", 0)


def _node(profile: Dict[str, Any], entry: ShareEntry) -> Dict[str, Any]:
    if entry.is_dir:
        return media_contract.node(profile, "folder", entry.url, entry.name, "", False, True)
    item = _track(profile, entry.url)
    return media_contract.node(
        profile,
        "tracks",
        item["id"],
        item["title"],
        item["artist"],
        True,
        False,
        item["artist"],
        item["album"],
        item["duration_seconds"],
    )


def _basic_headers(profile: Dict[str, Any]) -> Dict[str, str]:
    username = profile.get("username") or ""
    password = profile.get("password") or profile.get("api_token") or ""
    if not username and not password:
        return {}
    import base64

    token = base64.b64encode(f"{username}:{password}".encode("utf-8")).decode("ascii")
    return {"Authorization": f"Basic {token}"}


def _webdav_list(profile: Dict[str, Any], url: str) -> List[ShareEntry]:
    request = urllib.request.Request(url, method="PROPFIND", headers={**_basic_headers(profile), "Depth": "1"})
    with urllib.request.urlopen(request, timeout=20) as response:
        body = response.read()
    root = ET.fromstring(body)
    ns = {"d": "DAV:"}
    parsed_base = urllib.parse.urlparse(url)
    base_path = parsed_base.path.rstrip("/") + "/"
    result: List[ShareEntry] = []
    for response in root.findall(".//d:response", ns):
        href = response.findtext("d:href", default="", namespaces=ns)
        if not href:
            continue
        absolute = urllib.parse.urljoin(url, href)
        parsed = urllib.parse.urlparse(absolute)
        if parsed.path.rstrip("/") == parsed_base.path.rstrip("/"):
            continue
        collection = response.find(".//d:collection", ns) is not None
        name = posixpath.basename(urllib.parse.unquote(parsed.path.rstrip("/")))
        result.append(ShareEntry(absolute, name, collection))
    return result


def _ftp_connect(profile: Dict[str, Any]) -> tuple[ftplib.FTP, str]:
    parsed = urllib.parse.urlparse(base_url(profile).replace("http://", "ftp://", 1).replace("https://", "ftp://", 1))
    ftp = ftplib.FTP()
    ftp.connect(parsed.hostname or "", parsed.port or 21, timeout=20)
    ftp.login(profile.get("username") or "anonymous", profile.get("password") or profile.get("api_token") or "anonymous@")
    return ftp, urllib.parse.unquote(parsed.path or "/")


def _ftp_url(profile: Dict[str, Any], path: str) -> str:
    parsed = urllib.parse.urlparse(base_url(profile).replace("http://", "ftp://", 1).replace("https://", "ftp://", 1))
    netloc = parsed.hostname or ""
    if parsed.port:
        netloc += f":{parsed.port}"
    if profile.get("username"):
        user = urllib.parse.quote(profile.get("username", ""))
        password = urllib.parse.quote(profile.get("password") or profile.get("api_token") or "")
        netloc = f"{user}:{password}@{netloc}" if password else f"{user}@{netloc}"
    return urllib.parse.urlunparse(("ftp", netloc, urllib.parse.quote(path), "", "", ""))


def _ftp_list(profile: Dict[str, Any], url: str) -> List[ShareEntry]:
    ftp, root = _ftp_connect(profile)
    try:
        parsed = urllib.parse.urlparse(url)
        path = urllib.parse.unquote(parsed.path) if parsed.scheme == "ftp" else root
        entries: List[ShareEntry] = []
        try:
            for name, facts in ftp.mlsd(path):
                if name in (".", ".."):
                    continue
                child_path = posixpath.join(path.rstrip("/"), name)
                entries.append(ShareEntry(_ftp_url(profile, child_path), name, facts.get("type") == "dir"))
            return entries
        except ftplib.error_perm:
            pass

        original = ftp.pwd()
        for item in ftp.nlst(path):
            if item in (".", ".."):
                continue
            name = posixpath.basename(item.rstrip("/")) or item
            if name in (".", ".."):
                continue
            child_path = item if item.startswith("/") else posixpath.join(path.rstrip("/"), item)
            is_dir = False
            try:
                ftp.cwd(child_path)
                is_dir = True
            except ftplib.error_perm:
                is_dir = False
            finally:
                try:
                    ftp.cwd(original)
                except ftplib.error_perm:
                    ftp.cwd("/")
            entries.append(ShareEntry(_ftp_url(profile, child_path), name, is_dir))
        return entries
    finally:
        ftp.quit()


def _sftp_parts(profile: Dict[str, Any], url: Optional[str] = None) -> tuple[str, str, str]:
    parsed = urllib.parse.urlparse(url or base_url(profile).replace("http://", "sftp://", 1).replace("https://", "sftp://", 1))
    username = profile.get("username") or parsed.username or ""
    host = parsed.hostname or ""
    remote = urllib.parse.unquote(parsed.path or "/")
    target = f"{username}@{host}" if username else host
    return target, remote, parsed.hostname or ""


def _ssh_json(target: str, script: str) -> Any:
    command = "python3 -c " + shlex.quote(script)
    output = subprocess.check_output(
        ["ssh", "-o", "BatchMode=yes", "-o", "StrictHostKeyChecking=accept-new", target, command],
        text=True,
        timeout=25,
    )
    return json.loads(output)


def _sftp_url(profile: Dict[str, Any], host: str, path: str) -> str:
    username = profile.get("username") or ""
    netloc = f"{urllib.parse.quote(username)}@{host}" if username else host
    return urllib.parse.urlunparse(("sftp", netloc, urllib.parse.quote(path), "", "", ""))


def _sftp_list(profile: Dict[str, Any], url: str) -> List[ShareEntry]:
    target, remote, host = _sftp_parts(profile, url)
    script = (
        "import json, os, sys; p="
        + repr(remote)
        + "; out=[]\n"
        "for name in sorted(os.listdir(p)):\n"
        "    if name.startswith('.'): continue\n"
        "    q=os.path.join(p,name)\n"
        "    out.append({'name':name,'path':q,'dir':os.path.isdir(q)})\n"
        "print(json.dumps(out, ensure_ascii=False))"
    )
    return [ShareEntry(_sftp_url(profile, host, item["path"]), item["name"], bool(item["dir"])) for item in _ssh_json(target, script)]


def _sftp_find(profile: Dict[str, Any], limit: int) -> List[str]:
    target, remote, host = _sftp_parts(profile)
    script = (
        "import json, os; root="
        + repr(remote)
        + "; out=[]; exts="
        + repr(AUDIO_EXTENSIONS)
        + "\n"
        "for base, dirs, files in os.walk(root):\n"
        "    dirs[:] = [d for d in dirs if not d.startswith('.')]\n"
        "    for name in sorted(files):\n"
        "        if name.lower().endswith(exts): out.append(os.path.join(base,name))\n"
        "        if len(out) >= "
        + str(limit)
        + ": break\n"
        "    if len(out) >= "
        + str(limit)
        + ": break\n"
        "print(json.dumps(out, ensure_ascii=False))"
    )
    return [_sftp_url(profile, host, path) for path in _ssh_json(target, script)]


def _file_list(profile: Dict[str, Any], url: str) -> List[ShareEntry]:
    import os

    parsed = urllib.parse.urlparse(url)
    path = urllib.parse.unquote(parsed.path if parsed.scheme == "file" else profile.get("base_url", ""))
    result: List[ShareEntry] = []
    for name in sorted(os.listdir(path)):
        if name.startswith("."):
            continue
        child = os.path.join(path, name)
        result.append(ShareEntry(urllib.parse.urlunparse(("file", "", urllib.parse.quote(child), "", "", "")), name, os.path.isdir(child)))
    return result


def _list(profile: Dict[str, Any], url: Optional[str] = None) -> List[ShareEntry]:
    kind = _kind(profile)
    current = url or base_url(profile)
    if kind == "webdav":
        return _webdav_list(profile, current)
    if kind == "ftp":
        return _ftp_list(profile, current)
    if kind == "sftp":
        return _sftp_list(profile, current)
    if current.startswith("file://") or kind in ("nfs", "smb"):
        return _file_list(profile, current if current.startswith("file://") else profile.get("base_url", ""))
    return []


def _walk(profile: Dict[str, Any], limit: int) -> List[str]:
    kind = _kind(profile)
    if kind == "sftp":
        return _sftp_find(profile, limit)
    result: List[str] = []
    stack = [base_url(profile)]
    seen = set()
    while stack and len(result) < limit:
        current = stack.pop()
        if current in seen:
            continue
        seen.add(current)
        for entry in _list(profile, current):
            if entry.is_dir:
                stack.append(entry.url)
            elif _is_audio(entry.url):
                result.append(entry.url)
                if len(result) >= limit:
                    break
    return result


def probe(profile: Dict[str, Any]) -> Dict[str, Any]:
    try:
        entries = _list(profile)
    except Exception as exc:
        return {
            "available": False,
            "server_name": profile.get("name", "Share"),
            "user_id": profile.get("username", ""),
            "access_token": "",
            "message": type(exc).__name__,
        }
    return {
        "available": True,
        "server_name": profile.get("name", "Share"),
        "user_id": profile.get("username", ""),
        "access_token": "",
        "message": f"{len(entries)} ENTRIES",
    }


def search(profile: Dict[str, Any], session: Dict[str, Any], query: str, limit: int) -> List[Dict[str, Any]]:
    del session
    needle = query.casefold()
    tracks = [_track(profile, url) for url in _walk(profile, max(limit, 200))]
    return [item for item in tracks if not needle or needle in item.get("title", "").casefold() or needle in item.get("artist", "").casefold() or needle in item.get("album", "").casefold()][:limit]


def recent(profile: Dict[str, Any], session: Dict[str, Any], limit: int) -> List[Dict[str, Any]]:
    del session
    return [_track(profile, url) for url in _walk(profile, limit)]


def library_tracks(profile: Dict[str, Any], session: Dict[str, Any], limit: int) -> List[Dict[str, Any]]:
    del session
    return [_track(profile, url) for url in _walk(profile, limit)]


def browse(profile: Dict[str, Any], session: Dict[str, Any], parent: Dict[str, Any], limit: int) -> List[Dict[str, Any]]:
    del session
    url = parent.get("id") if parent.get("kind") == "folder" else base_url(profile)
    return [_node(profile, entry) for entry in _list(profile, url)[:limit] if entry.is_dir or _is_audio(entry.url)]


def resolve(profile: Dict[str, Any], session: Dict[str, Any], track: Dict[str, Any]) -> Dict[str, Any]:
    del profile, session
    url = track.get("id", "")
    return {
        "url": url,
        "headers": [],
        "seekable": True,
        "connected": bool(url),
        "connection_status": "READY" if url else "MISSING URL",
    }
