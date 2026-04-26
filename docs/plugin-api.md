<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Plugin And Provider API

Normative provider rules live in `docs/specification/plugin-provider-system-spec.md`.

## Provider Responsibilities

Each provider is responsible for:

- configuration model
- authentication model
- connection lifecycle
- directory/catalog enumeration
- search
- track/station resolution
- media stream acquisition
- source-specific error mapping
- timeout and cache strategy
- provider-specific tests

## Host Responsibilities

The host owns:

- provider discovery and registration
- capability projection
- credential-store mediation
- logging and redaction
- mapping into shared Track, Album, Artist, Playlist, Station, MediaItem, and MediaStream models

## ABI Posture

Binary plugins must not accidentally make unstable C++ ABI public.
Until a stable binary boundary is designed, providers may be built internally behind a stable source-level provider API.
