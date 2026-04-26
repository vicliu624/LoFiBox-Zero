<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Protocol Sources

Normative streaming rules live in `docs/specification/lofibox-zero-streaming-spec.md`.

## Required Source Families

LoFiBox must model:

- local files
- direct remote URLs
- internet radio and station entries
- SMB
- NFS
- WebDAV
- FTP
- SFTP
- Jellyfin
- Navidrome
- Emby
- Subsonic
- OpenSubsonic
- DLNA
- UPnP

## Unified Model

Protocol data must map into common product objects:

- `Track`
- `Album`
- `Artist`
- `Playlist`
- `Station`
- `MediaItem`
- `MediaStream`

Protocol names must not become UI or playback truth.

## Per-Source Contract

Each source family needs:

- config model
- auth model
- connection lifecycle
- catalog enumeration
- search
- stream resolution
- error model
- timeout model
- cache strategy
- tests

## First-Batch Provider Runtime

The first-batch runtime providers are:

- `Jellyfin`
- `Emby`
- `OpenSubsonic`
- `Navidrome` as an OpenSubsonic-compatible provider family

These providers must not be capability placeholders.
Their host runtime path must support authenticated probe, catalog-track enumeration, search, latest or recent lookup, and stream resolution through `RemoteMediaServices`.
Mock tests must stay isolated from real user servers, while real-server smoke tests may exist only as opt-in tests driven by environment variables.
Real credentials must not be stored in source files, profile persistence, test fixtures, logs, or packaging artifacts.
