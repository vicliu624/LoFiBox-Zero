<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Integrated Product Core Specification

## Purpose

This specification defines the integrated product core for LoFiBox.

It exists because playback, audio pipeline, metadata enrichment, library facts, remote sources, credentials, UI projections, and desktop/device shells cannot be governed as isolated feature patches. They share the same product truth: a media item enters LoFiBox from a source, is identified, indexed, resolved to a stream, played through one audio pipeline, projected to UI/desktop surfaces, and persisted under Linux/XDG/Debian constraints.

This document is normative for the current C++ implementation.

## Core Distinctions

- `MediaIdentity`: the evidence and confidence that a local file, remote item, server item, or stream represents a particular track.
- `MediaSource`: the origin of media catalog entries and streams, such as local files, direct URLs, radio, Jellyfin, Navidrome/OpenSubsonic, Emby, or future shares.
- `MediaStream`: the resolved playable stream consumed by playback and audio code.
- `PlaybackFact`: current-track, queue, elapsed time, duration, backend state, transition generation, and completion event truth.
- `AudioPipeline`: decoder, PCM frame flow, DSP chain, output sink, and visualization tap.
- `EnrichmentPipeline`: metadata, artwork, lyrics, identity, confidence, cache, and writeback decisions.
- `LibraryFact`: indexed tracks, albums, artists, playlists, source mapping, scan generations, play counts, and recently-played facts.
- `UiProjection`: page, widget, desktop, and device-facing render/control models derived from product facts.
- `RuntimeShell`: Linux host, device profile, desktop, container, VNC, or PocketFrame shell. Runtime shells adapt input/output; they do not redefine product facts.
- `RuntimeServiceProvider`: host-side construction of service groups before they enter the shared app as injected capabilities.

## Mandatory Ownership

- `PlaybackController` owns playback commands and queue-facing state-machine semantics only.
- `PlaybackSessionClock` owns elapsed/duration/paused/finished time facts.
- `PlaybackBackendController` owns playback backend command sequencing.
- `PlaybackTransitionCoordinator` owns atomic previous/next/natural-finish transition orchestration.
- `PlaybackCompletionPolicy` owns repeat/shuffle/end-of-queue decisions.
- `AudioPipeline` owns decoder-to-sink frame flow and DSP/visualization taps.
- `VisualizationSource` owns the app-facing spectrum frame fact.
- `TrackIdentityResolver` owns identity evidence and confidence calculation.
- `MetadataResolverChain`, `ArtworkResolverChain`, and `LyricsResolverChain` own ordered lookup chains.
- `MetadataEnrichmentOrchestrator` owns online metadata lookup ordering and fallback strategy.
- `MetadataWritebackPolicy` owns whether enrichment results may mutate audio files.
- `MatchConfidenceGuard` owns whether an online result is trusted enough to accept.
- `LibraryStore` and `LibraryIndexer` own durable media-library facts.
- `LibraryRepository` owns current in-memory library facts and scan replacement state.
- `RemoteSourceRegistry` owns source provider registration and lookup.
- `CredentialStore`, `CredentialRef`, `SecretRedactor`, and `TlsPolicy` own secret and network security boundaries.
- `RuntimeServiceProvider` owns host service group construction; the app consumes only the completed `RuntimeServices` registry.
- `AppProjectionBuilder` and page-specific projection builders own app-to-UI view-model assembly.
- UI pages and widgets own layout and visual composition only.
- Desktop integration owns event/notification adapters only and converts external events into product commands.

## Invalid Ownership

- UI pages must not call FFmpeg, curl, Jellyfin, SQLite, D-Bus, or source protocol clients.
- Playback code must not know Jellyfin, SMB, Navidrome, OpenSubsonic, Emby, DLNA, WebDAV, FTP, SFTP, or concrete remote protocols.
- Remote providers must not control playback state machines.
- Metadata, artwork, and lyrics providers must not decide UI behavior.
- Metadata providers must not inline online identity/MusicBrainz lookup ordering; they delegate enrichment orchestration.
- `PlaybackController` must not own backend clocks, backend threads, async enrichment workers, or completion policy.
- `AudioPipeline` must not own queue selection or source browsing.
- `LyricsProvider` must not hide cache, writeback, and match-confidence policy in one body.
- `LibraryController` must not own database migration, scan scheduling, remote provider calls, or UI row rendering.
- `LibraryController` must not own raw library fact storage when a repository boundary exists.
- Runtime shells must not fork business logic for PocketFrame, Cardputer Zero, container, VNC, framebuffer, X11, or desktop widget targets.
- Runtime service factories must not regain protocol, metadata, playback, cache, or remote implementation details.
- Credentials, tokens, cookies, API keys, and auth headers must not be logged or stored as plain app state.

## Required Source Families

The integrated core must account for these source families even when some providers are still evolving:

- `LocalFileSource`
- `DirectUrlSource`
- `RadioStationSource`
- `JellyfinSource`
- `OpenSubsonicSource`
- `NavidromeSource`
- `EmbySource`
- future `SmbSource`, `NfsSource`, `WebDavSource`, `FtpSource`, `SftpSource`, `DlnaUpnpSource`

Every source must eventually define configuration, authentication, connection lifecycle, catalog enumeration, search, stream resolution, error model, timeout policy, cache policy, and test strategy.

## Acceptance Rules

- A playable item must be representable without knowing whether it came from local storage or a remote server.
- A stream must be resolved before audio playback consumes it.
- The audio pipeline is the source of real visualization data.
- Metadata enrichment must distinguish lookup, acceptance, cache, and writeback.
- Existing embedded metadata must not be overwritten by weaker online evidence.
- Library facts must survive UI navigation changes.
- Runtime paths must follow XDG and must not write user state into install directories.
- Debian package validation must remain green after core boundary work.

## Required Machine Gates

`scripts/check-architecture-boundaries.ps1` must reject regressions where:

- `PlaybackController` regains clock, backend, transition, completion, or enrichment-worker ownership.
- UI includes app/backend/protocol layers.
- playback includes remote/protocol layers.
- remote includes playback/UI layers.
- metadata/enrichment includes UI pages.
- `AppRenderer` regains projection-building helpers.
- `LyricsPage` regains parsing or spectrum algorithms.
- `LibraryController` regains list-open branching, database, or remote provider ownership.
- host runtime protocol clients collapse into monolithic helper files.
- runtime service factory code directly constructs concrete metadata/playback/remote services instead of composing service providers.

## Relation To Other Specifications

- Final product meaning remains in `lofibox-zero-final-product-spec.md`.
- Media pipeline details remain in `lofibox-zero-media-pipeline-spec.md`.
- DSP details remain in `lofibox-zero-audio-dsp-spec.md`.
- Streaming source details remain in `lofibox-zero-streaming-spec.md`.
- Track identity details remain in `lofibox-zero-track-identity-spec.md`.
- Library indexing details remain in `lofibox-zero-library-indexing-spec.md`.
- Credentials remain in `lofibox-zero-credential-spec.md`.
- Debian and dependency governance remain in `debian-official-archive-spec.md` and `dependency-policy-spec.md`.
