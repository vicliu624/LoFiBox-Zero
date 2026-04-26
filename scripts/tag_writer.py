#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later

import base64
import json
import mimetypes
import os
import sys
from pathlib import Path

import mutagen
from mutagen.flac import FLAC, Picture
from mutagen.id3 import APIC, ID3, ID3NoHeaderError, TALB, TCOM, TCON, TIT2, TPE1, TXXX, USLT
from mutagen.mp3 import MP3
from mutagen.oggopus import OggOpus
from mutagen.oggvorbis import OggVorbis


def _non_empty(values):
    if values is None:
        return False
    if isinstance(values, str):
        return bool(values.strip())
    return any(str(value).strip() for value in values)


def _mime_for_path(path: Path) -> str:
    try:
        header = path.read_bytes()[:12]
    except OSError:
        header = b""
    if header.startswith(b"\x89PNG\r\n\x1a\n"):
        return "image/png"
    if header.startswith(b"\xff\xd8\xff"):
        return "image/jpeg"
    mime, _ = mimetypes.guess_type(path.name)
    return mime or "image/png"


def _read_payload(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def _write_mp3(target: Path, payload: dict) -> bool:
    metadata = payload.get("metadata") or {}
    identity = payload.get("identity") or {}
    lyrics = payload.get("lyrics") or {}
    artwork_path = payload.get("artwork_png_path")
    only_fill_missing = bool(payload.get("only_fill_missing", True))
    clear_missing_metadata = bool(payload.get("clear_missing_metadata", False))

    try:
        tags = ID3(target)
    except ID3NoHeaderError:
        tags = ID3()

    def maybe_set_text(frame_id, frame_cls, value):
        if not value:
            if clear_missing_metadata and not only_fill_missing:
                tags.delall(frame_id)
            return
        existing = tags.getall(frame_id)
        has_existing = any(_non_empty(getattr(frame, "text", [])) for frame in existing)
        if only_fill_missing and has_existing:
            return
        tags.delall(frame_id)
        tags.add(frame_cls(encoding=3, text=[value]))

    maybe_set_text("TIT2", TIT2, metadata.get("title"))
    maybe_set_text("TPE1", TPE1, metadata.get("artist"))
    maybe_set_text("TALB", TALB, metadata.get("album"))
    maybe_set_text("TCON", TCON, metadata.get("genre"))
    maybe_set_text("TCOM", TCOM, metadata.get("composer"))

    def maybe_set_txxx(desc, value):
        if not value:
            return
        frame_id = f"TXXX:{desc}"
        existing = tags.getall(frame_id)
        has_existing = any(_non_empty(getattr(frame, "text", [])) for frame in existing)
        if only_fill_missing and has_existing:
            return
        tags.delall(frame_id)
        tags.add(TXXX(encoding=3, desc=desc, text=[value]))

    maybe_set_txxx("MusicBrainz Track Id", identity.get("recording_mbid"))
    maybe_set_txxx("MusicBrainz Album Id", identity.get("release_mbid"))
    maybe_set_txxx("MusicBrainz Release Group Id", identity.get("release_group_mbid"))
    maybe_set_txxx("LoFiBox Identity Source", identity.get("source"))

    plain_lyrics = lyrics.get("plain")
    if plain_lyrics:
        has_existing = any(_non_empty(getattr(frame, "text", "")) for frame in tags.getall("USLT"))
        if not (only_fill_missing and has_existing):
            tags.delall("USLT")
            tags.add(USLT(encoding=3, lang="eng", desc="", text=plain_lyrics))

    synced_lyrics = lyrics.get("synced")
    if synced_lyrics:
        has_existing = any(_non_empty(getattr(frame, "text", [])) for frame in tags.getall("TXXX:SYNCEDLYRICS"))
        if not (only_fill_missing and has_existing):
            tags.delall("TXXX:SYNCEDLYRICS")
            tags.add(TXXX(encoding=3, desc="SYNCEDLYRICS", text=[synced_lyrics]))

    if artwork_path:
        art_path = Path(artwork_path)
        has_existing_art = len(tags.getall("APIC")) > 0
        if art_path.exists() and not (only_fill_missing and has_existing_art):
            tags.delall("APIC")
            tags.add(
                APIC(
                    encoding=3,
                    mime=_mime_for_path(art_path),
                    type=3,
                    desc="Cover",
                    data=art_path.read_bytes(),
                )
            )

    tags.save(target, v2_version=3)
    return True


def _write_flac_like(audio, payload: dict) -> bool:
    metadata = payload.get("metadata") or {}
    identity = payload.get("identity") or {}
    lyrics = payload.get("lyrics") or {}
    artwork_path = payload.get("artwork_png_path")
    only_fill_missing = bool(payload.get("only_fill_missing", True))
    clear_missing_metadata = bool(payload.get("clear_missing_metadata", False))

    def maybe_set_field(key, value):
        if not value:
            if clear_missing_metadata and not only_fill_missing:
                try:
                    del audio[key]
                except KeyError:
                    pass
            return
        existing = audio.get(key, [])
        if only_fill_missing and _non_empty(existing):
            return
        audio[key] = [value]

    maybe_set_field("title", metadata.get("title"))
    maybe_set_field("artist", metadata.get("artist"))
    maybe_set_field("album", metadata.get("album"))
    maybe_set_field("genre", metadata.get("genre"))
    maybe_set_field("composer", metadata.get("composer"))
    maybe_set_field("musicbrainz_trackid", identity.get("recording_mbid"))
    maybe_set_field("musicbrainz_albumid", identity.get("release_mbid"))
    maybe_set_field("musicbrainz_releasegroupid", identity.get("release_group_mbid"))
    maybe_set_field("lofibox_identity_source", identity.get("source"))
    maybe_set_field("lyrics", lyrics.get("plain"))
    maybe_set_field("syncedlyrics", lyrics.get("synced"))

    if artwork_path:
        art_path = Path(artwork_path)
        has_existing_art = bool(getattr(audio, "pictures", [])) or _non_empty(audio.get("metadata_block_picture", []))
        if art_path.exists() and not (only_fill_missing and has_existing_art):
            data = art_path.read_bytes()
            pic = Picture()
            pic.type = 3
            pic.mime = _mime_for_path(art_path)
            pic.desc = "Cover"
            pic.data = data

            if hasattr(audio, "clear_pictures"):
                audio.clear_pictures()
            if hasattr(audio, "add_picture"):
                audio.add_picture(pic)
            else:
                encoded = base64.b64encode(pic.write()).decode("ascii")
                audio["metadata_block_picture"] = [encoded]

    audio.save()
    return True


def write_tags(payload: dict) -> bool:
    target = Path(payload["path"])
    if not target.exists():
        return False

    audio = mutagen.File(target, easy=False)
    if audio is None:
        return False

    if isinstance(audio, MP3):
        return _write_mp3(target, payload)
    if isinstance(audio, FLAC):
        return _write_flac_like(audio, payload)
    if isinstance(audio, OggVorbis):
        return _write_flac_like(audio, payload)
    if isinstance(audio, OggOpus):
        return _write_flac_like(audio, payload)

    return False


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        return 2
    payload = _read_payload(Path(argv[1]))
    return 0 if write_tags(payload) else 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
