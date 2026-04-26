<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Plugin And Provider System Specification

## 1. Purpose

This document defines how `LoFiBox Zero` organizes optional and protocol-specific capabilities without deleting product functionality or leaking protocol details into UI and playback.

## 2. Provider Model

Remote and protocol capabilities should be represented as providers behind a stable host API.

The host owns:

- provider registration
- provider capability discovery
- lifecycle management
- credential-store access boundaries
- logging and redaction policy
- integration with shared `Track`, `Album`, `Artist`, `Playlist`, `Station`, `MediaItem`, and `MediaStream` models

The provider owns:

- source-specific configuration model
- authentication model
- connection lifecycle
- catalog enumeration
- search
- track or station resolution
- stream URL or stream-handle retrieval
- source-specific error mapping
- timeout policy
- cache policy hints
- protocol-specific tests

## 3. Required Provider Families

The source tree must preserve space for:

- `LocalFileSource`
- `DirectUrlSource`
- `RadioStationSource`
- `SmbSource`
- `NfsSource`
- `WebDavSource`
- `FtpSource`
- `SftpSource`
- `JellyfinSource`
- `NavidromeSource`
- `EmbySource`
- `SubsonicSource`
- `OpenSubsonicSource`
- `DlnaUpnpSource`

## 4. ABI Strategy

Binary plugin boundaries must not accidentally expose unstable C++ ABI as a public contract.

Acceptable strategies include:

- internally compiled providers with a stable source-level provider API
- binary providers with a C ABI host boundary
- manifest-driven provider discovery with explicit version and capability declarations

The chosen strategy must be documented before binary plugin distribution is treated as stable.

## 5. Future Package Split

The source tree must keep the complete product capability model.
Distribution packaging may later split protocol providers into packages such as:

- `lofibox`
- `lofibox-plugin-smb`
- `lofibox-plugin-nfs`
- `lofibox-plugin-webdav`
- `lofibox-plugin-ftp`
- `lofibox-plugin-sftp`
- `lofibox-plugin-jellyfin`
- `lofibox-plugin-navidrome`
- `lofibox-plugin-emby`
- `lofibox-plugin-subsonic`
- `lofibox-plugin-dlna`

This is package organization, not product shrinkage.

## 6. Boundary Rules

- Providers must not control UI pages directly.
- Providers must not control playback state machines directly.
- Providers must not bypass the credential manager.
- Providers must map external protocol data into shared product models.
- Providers must expose capability facts instead of requiring pages to infer behavior from protocol names.
