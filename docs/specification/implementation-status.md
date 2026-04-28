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
- direct Linux X11 desktop-widget target, with `linux-x11-debug` and `linux-x11-debug-build` CMake presets
- host-machine Chromaprint/fpcalc provisioning through `scripts/ensure-host-fpcalc.ps1`; the current Windows host has `fpcalc` 1.6.0 installed through winget

## 3. Current Device-Side Scope

- Linux framebuffer device target
- Linux `evdev` keyboard input path
- direct Linux framebuffer output adapter
- host runtime capability adapters injected behind app-facing interfaces

## 4. Still Not Locally Completable

- LAN share and DLNA/UPnP provider families now resolve configured stream URLs through the unified remote stream path; richer native directory discovery still depends on optional backend/platform adapters.
- pointer-facing or large-screen final UI surfaces for advanced/professional DSP editing require an accepted non-small-screen device profile. The domain model and runtime chain state exist; the current required small-screen profile exposes the compact 10-band surface.
- Real Linux-session MPRIS service, D-Bus service registration, global media-key capture, and notification delivery remain platform-adapter work beyond the current app-side desktop projection/state boundary.
- Debian archive upload readiness still requires external maintainer identity, release tag, ITP/changelog closure, and sponsor-style clean chroot proof as tracked in `docs/engineering-check-report.md`.

## 5. Explicitly Excluded

- SDL or mock desktop simulator support is not a missing implementation item. `project-architecture-spec.md` removes that product/runtime distinction; direct host visual validation belongs to the real Linux X11 desktop-widget target.

## 6. Rule

Nothing in this file may be treated as the final product definition merely because it reflects what is implemented today.


## 2026-04-27 Functional Convergence Update

This update records implementation, not just domain modeling:

- Source Manager selections now enter real remote behavior: configured profiles probe, open Remote Browse, enumerate provider catalog nodes, resolve playable nodes, show Stream Detail, and start remote playback through the shared audio backend URI path.
- Direct URL, internet radio, playlist manifest, HLS/DASH, SMB, NFS, WebDAV, FTP, SFTP, and DLNA/UPnP source kinds round-trip through persisted profile kind strings and can be resolved as configured stream URIs.
- Jellyfin, Emby, OpenSubsonic, and Navidrome provider tooling exposes browse actions for root categories, artists, albums, playlists, folders where applicable, favorites, recent views, and playable tracks.
- Search is now an interactive page: typed characters build the query, local and active remote results are grouped by source, and confirming a result starts local or remote playback.
- Stream Detail is now actionable: it shows resolved diagnostics and `ENTER` starts the resolved stream.
- Queue / Up Next is reachable from Now Playing with `Q` and displays current local or remote playback state.
- Playlist Editor is reachable from playlist pages with `E` or `INS` so `F4` remains the global previous-track transport key, and the editor has its own page identity/help ownership.
- Desktop-open positional file/URL arguments are accepted by CLI targets and routed into local library startup playback or direct remote stream playback.
- Library rescan now records incremental change facts and migration planning in `LibraryRepository` rather than leaving governance only as a standalone model.
- Metadata governance now supports a persistent fingerprint index path in addition to in-memory lookup.
- Playback runtime now consumes the stability policy for transition preparation and start/end jitter suppression.
- `lofibox_remote_browse_playback_flow_smoke` covers Source Manager -> Remote Browse -> Stream Detail -> Now Playing remote URI playback.
- Remote browse/search playback now carries remote title, artist, album, and duration into Now Playing/Lyrics projections, and read-only remote items can reuse accepted local metadata cache records keyed by stable source identity plus item id.
- Host fingerprint provisioning is now executable outside the container: `scripts/ensure-host-fpcalc.ps1` detects `FPCALC_PATH` or `fpcalc`, can install Chromaprint through supported package managers, and was verified on this Windows host through winget.
- Direct Linux desktop-widget builds are no longer implicit knowledge: `linux-x11-debug` configures the real X11 presentation target.

## 2026-04-28 Search And Text Input Specification Update

This update records specification convergence before the next implementation pass:

- Search is a current first-class small-screen page, not a deferred feature.
- Editable text is now governed by `lofibox-zero-text-input-spec.md`.
- Search query truth is committed UTF-8 app state; input-method preedit remains transient projection and must not mutate query truth.
- Debian/Linux CJK input belongs to the user's system/session input-method stack and enters LoFiBox through committed UTF-8 text events.
- The X11 desktop-widget target must integrate with system input methods; framebuffer/evdev remains a direct Linux-key input adapter unless a separate device input-method design is specified.
- Current implementation still needs to converge from character/ASCII-oriented input plumbing to committed UTF-8 text events, Unicode-safe backspace, and tested non-ASCII Search matching before CJK search is product-grade.
