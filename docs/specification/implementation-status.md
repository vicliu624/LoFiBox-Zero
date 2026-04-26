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
- implementation placement gate for source ownership: first-batch remote provider behavior lives under `src/remote/*`, playback implementation lives under `src/playback`, audio output/decoder contracts live under `src/audio/*`, and helper entrypoints cannot hide protocol implementation details
- host single-instance startup guard for the Linux device executable

## 3. Current Device-Side Scope

- Linux framebuffer device target
- Linux `evdev` keyboard input path
- direct Linux framebuffer output adapter
- host runtime capability adapters injected behind app-facing interfaces

## 4. Still Not Implemented

- final-form streaming UI surfaces
- final-form DSP and EQ management surfaces
- local persistent fingerprint index
- host-machine provisioning outside the development container for optional fingerprint dependency `fpcalc`
- direct on-host desktop simulator support
- full remote hierarchy pages for server artists, albums, playlists, folders, genres, favorites, and server-side writeback actions

## 5. Rule

Nothing in this file may be treated as the final product definition merely because it reflects what is implemented today.
