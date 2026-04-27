<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Implementation Status

## 1. Purpose

This document describes current progress only.

It is intentionally not a product-definition document.
Use `lofibox-zero-final-product-spec.md` for final product truth.
Use `project-architecture-spec.md` for enduring architecture rules.

## 2. Current User-Visible Scope

- shared `320x170` player UI with menu, library browsing, now-playing, lyrics, EQ, and settings surfaces
- local media-root scan and playable queue behavior
- metadata, artwork, lyrics, and tag writeback runtime services
- track identity runtime service with MusicBrainz fallback and optional Chromaprint/AcoustID fingerprint path
- remote media runtime provider path for Jellyfin, OpenSubsonic/Navidrome-compatible servers, and Emby, including probe, catalog/search/recent lookup, and stream resolution behind app-facing remote service interfaces
- expanded streaming source model for direct URLs, internet radio, playlist manifests, HLS/DASH, SMB, NFS, WebDAV, FTP, SFTP, and DLNA/UPnP provider families, with manifest capabilities and protocol classification
- remote catalog hierarchy model for artists, albums, tracks, genres, playlists, folders, favorites, recently added, recently played, and stations
- streaming playback policy for buffer thresholds, reconnect/retry recovery, next-item and album/playlist prefetch, quality/transcode preference, and stream diagnostics
- final page-model surfaces for Search, Queue/Up Next, Remote Browse, Server Diagnostics, Stream Detail, and Playlist Editor
- implementation placement gate for source ownership: first-batch remote provider behavior lives under `src/remote/*`, playback implementation lives under `src/playback`, audio output/decoder contracts live under `src/audio/*`, and helper entrypoints cannot hide protocol implementation details
- unified `CacheManager` for playback, artwork, metadata, lyrics, remote-directory, station-list, recent-browse, and offline-audio cache buckets, including capacity/age policy, garbage collection, and album/playlist offline sync planning
- final-form DSP domain objects for built-in/user/device/content-specific EQ profiles, preset CRUD/import/export, device/content bindings, loudness, balance, limiter, ReplayGain mode, parametric bands, high-pass/low-pass filters, and smoothing helpers
- media-library governance service for incremental change detection, database migration planning, smart playlists, source-grouped search result explanations, and duplicate/same-recording identity decisions
- metadata governance service for local fingerprint index entries, enrichment conflict reporting, artwork provenance priority, and writeback format coverage for ID3, FLAC/Vorbis, Ogg/Vorbis, and MP4-family tags
- playback stability policy for gapless/crossfade preparation, transition lead time, and start/end jitter suppression
- desktop runtime integration state for MPRIS/D-Bus/media-key/notification availability and desktop open-file/open-URL requests
- credential lifecycle and permission model for system-keyring-facing storage, token validity/revocation, read/write source capabilities, and safe status reporting
- host single-instance startup guard for the Linux device executable

## 3. Current Device-Side Scope

- Linux framebuffer device target
- Linux `evdev` keyboard input path
- direct Linux framebuffer output adapter
- host runtime capability adapters injected behind app-facing interfaces

## 4. Still Not Implemented

- host-machine provisioning outside the development container for optional fingerprint dependency `fpcalc`
- direct on-host desktop simulator support
- LAN share and DLNA/UPnP provider families now resolve configured stream URLs through the unified remote stream path; richer native directory discovery still depends on optional backend/platform adapters.\n- pointer-facing or large-screen final UI surfaces for advanced/professional DSP editing; the domain model and runtime chain state now exist, while current small-screen page still exposes the compact 10-band surface

## 5. Rule

Nothing in this file may be treated as the final product definition merely because it reflects what is implemented today.


## 2026-04-27 Functional Convergence Update

This update records implementation, not just domain modeling:

- Source Manager selections now enter real remote behavior: configured profiles probe, open Remote Browse, enumerate provider catalog nodes, resolve playable nodes, show Stream Detail, and start remote playback through the shared audio backend URI path.
- Direct URL, internet radio, playlist manifest, HLS/DASH, SMB, NFS, WebDAV, FTP, SFTP, and DLNA/UPnP source kinds round-trip through persisted profile kind strings and can be resolved as configured stream URIs.
- Jellyfin, Emby, OpenSubsonic, and Navidrome provider tooling exposes browse actions for root categories, artists, albums, playlists, folders where applicable, favorites, recent views, and playable tracks.
- Search is now an interactive page: typed characters build the query, local and active remote results are grouped by source, and confirming a result starts local or remote playback.
- Stream Detail is now actionable: it shows resolved diagnostics and `ENTER` starts the resolved stream.
- Queue / Up Next is reachable from Now Playing with `Q` and displays current local or remote playback state.
- Playlist Editor is reachable from playlist pages with `F4` and has its own page identity/help ownership.
- Desktop-open positional file/URL arguments are accepted by CLI targets and routed into local library startup playback or direct remote stream playback.
- Library rescan now records incremental change facts and migration planning in `LibraryRepository` rather than leaving governance only as a standalone model.
- Metadata governance now supports a persistent fingerprint index path in addition to in-memory lookup.
- Playback runtime now consumes the stability policy for transition preparation and start/end jitter suppression.
- `lofibox_remote_browse_playback_flow_smoke` covers Source Manager -> Remote Browse -> Stream Detail -> Now Playing remote URI playback.
