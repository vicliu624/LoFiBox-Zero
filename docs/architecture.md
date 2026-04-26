<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Architecture

This document is the engineering entry point for LoFiBox architecture.
Normative details live under `docs/specification/`.

## Product Truth

LoFiBox is a pure C++ Linux desktop music player targeting Debian and Ubuntu official repositories.
Cardputer Zero, PocketFrame, framebuffer, VNC, and other device-shaped environments are adapters or validation profiles, not the product's highest identity.

Primary specs:

- `docs/specification/lofibox-zero-final-product-spec.md`
- `docs/specification/debian-official-archive-spec.md`
- `docs/specification/project-architecture-spec.md`

## Target Source Boundaries

The long-term source layout is semantic rather than incidental:

- `src/core`: domain model, state, commands, errors, configuration
- `src/playback`: playback lifecycle, queue, seek, gapless, crossfade
- `src/audio`: decoder, output, DSP, EQ, ReplayGain, resampling
- `src/metadata`: tags, artwork, lyrics linkage, metadata cache
- `src/library`: scan, index, database, migrations, queries
- `src/playlist`: playlist model and parsers
- `src/remote`: remote source and media server providers
- `src/plugins`: provider API, manifests, registration, lifecycle
- `src/desktop`: MPRIS, D-Bus, media keys, notifications
- `src/platform`: XDG, file watching, platform adapters
- `src/security`: credentials, TLS, auth, sensitive data
- `src/ui`: pages, controls, view models, themes

## Hard Boundary Rules

- UI must not directly call FFmpeg, SMB, Jellyfin, SQLite, D-Bus, or protocol clients.
- UI/page code must not include concrete platform adapters or backend/protocol implementation layers.
- Core code must not include app, platform, UI, playback, audio, metadata, library, remote, desktop, or security layers.
- Host adapters implement runtime services and helper/resource resolution, but do not depend on concrete app/page classes.
- Targets compose app runners and platform adapters; they do not own page implementations or product behavior.
- Playback must not know protocol details.
- Remote providers produce media objects and streams; they do not own playback state machines.
- Metadata and library code do not depend on UI.
- Desktop integration translates external events into player commands.
- Protocol clients do not persist plaintext secrets.
- Runtime user data follows XDG paths.

The repository enforces include-direction rules with `scripts/check-architecture-boundaries.ps1`, wired into CI.

## Runtime Path Boundary

Runtime paths are resolved through the platform layer, not by feature code reading environment variables directly.
On Linux, LoFiBox uses:

- config: `$XDG_CONFIG_HOME/lofibox` or `~/.config/lofibox`
- data: `$XDG_DATA_HOME/lofibox` or `~/.local/share/lofibox`
- cache: `$XDG_CACHE_HOME/lofibox` or `~/.cache/lofibox`
- state: `$XDG_STATE_HOME/lofibox` or `~/.local/state/lofibox`
- runtime: `$XDG_RUNTIME_DIR/lofibox` or a temporary fallback for ephemeral locks

The cache store, runtime logger, script bridge payloads, and single-instance lock must use this path boundary.
Installed paths such as `/usr/share/lofibox` and `/usr/lib/lofibox` are read-only runtime resource locations and must not receive user data, cache files, logs, or state.

## Runtime Helper Boundary

Host runtime helper scripts are packaged application resources, not source-tree assumptions.
Runtime resolution order is:

1. `LOFIBOX_HELPER_DIR` for explicit test/development override
2. installed private helper directory such as `/usr/lib/lofibox`
3. source-tree `scripts/` directory for developer builds

Feature code must call the runtime helper resolver instead of embedding `scripts/tag_writer.py` or `scripts/remote_media_tool.py` paths directly.

## Data Flow

The intended high-level flow is:

```text
SourceProvider -> CatalogProvider -> MediaItem
MediaItem -> StreamResolver -> TrackSource / MediaStream
TrackSource -> Decoder -> DSP Chain -> Volume -> AudioOutput
MetadataProvider -> Track / Album / Artist / Artwork / Lyrics projection
Desktop/Input/UI -> PlayerCommand -> PlaybackSession
```

## Command Flow

UI, media keys, D-Bus, and device input must converge into player commands.
Commands update core playback, queue, library, source, and settings state through defined boundaries.

## Error Model

Errors should be mapped at the boundary where they occur:

- protocol errors become source/provider errors
- decode errors become media-pipeline errors
- credential errors become security/credential errors
- desktop integration errors remain adapter errors

User-facing errors should be explanatory without leaking secrets.
