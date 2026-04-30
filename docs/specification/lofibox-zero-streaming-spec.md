<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Streaming Specification

## 1. Purpose

This document defines the streaming capability domain for `LoFiBox Zero`.

It exists to stop future work from collapsing very different requirements into a vague `streaming` label.
In this project, streaming is not just `paste a URL and play`.
It is the product capability that lets `LoFiBox Zero` act as:

- a network audio endpoint
- a remote media library client
- a resilient network-aware playback system

Use this document when deciding whether a remote-media concept belongs in the product at all, what responsibility owns it, and how it is allowed to integrate with the existing local-player model.

For final product meaning, use `lofibox-zero-final-product-spec.md`.
For architecture placement, use `project-architecture-spec.md`.
For shared source selection, default-source truth, and saved-source state, use `lofibox-zero-app-state-spec.md`.
For persisted source profiles, streaming-server settings, credential references, and load/repair rules, use `lofibox-zero-persistence-spec.md`.
For credential references, secret storage, and runtime session material rules, use `lofibox-zero-credential-spec.md`.
For decode, metadata, and output-pipeline behavior after a source has been resolved, use `lofibox-zero-media-pipeline-spec.md`.
For EQ, `ReplayGain`, limiter, loudness, and other DSP behavior after decode, use `lofibox-zero-audio-dsp-spec.md`.
For address, account, token, and other editable source-profile fields, use `lofibox-zero-text-input-spec.md`.
For page-level UI contracts, use `lofibox-zero-page-spec.md`.
For legacy carry-forward intent, use `legacy-lofibox-product-guidance.md`.

## 2. Scope

This document covers:

- remote source types and server families
- unified media abstractions for local and remote playback
- catalog browse and search responsibilities
- stream resolution, buffering, reconnection, and quality selection
- metadata, artwork, cache, offline-save, and security behavior
- the responsibility boundaries that keep streaming work from turning into ad-hoc glue

This document does not define:

- final streaming page map or pixel layout
- decoder or library implementation choices
- transport backend selection for Linux
- exact release sequencing

## 3. Distinction Contract

### 3.1 Current Confusions

- `streaming` can mean a direct audio URL, a playlist manifest, a radio stream, a LAN share, or a full media-server integration.
- `source access`, `catalog browsing`, `stream resolution`, `playback buffering`, and `authentication` are often incorrectly treated as one responsibility.
- `LoFiBox Zero` is still offline-first, but Linux-capable builds now also need a legitimate remote-media extension.
- Not every remotely playable thing is a normal music track; live stations and continuous streams have different semantics from seekable library items.

### 3.2 Valid Distinctions

- Streaming is a first-class capability domain, not a single button.
- Local media and remote media should converge at shared queue, playback, metadata, and DSP entry points when their semantics match.
- `Track`, `Album`, `Artist`, `Playlist`, and `Station` are product objects; `HTTP`, `SMB`, `HLS`, `DLNA`, and `Subsonic` are transport or source families.
- Connection management, catalog access, stream resolution, playback, metadata, cache, and security are separate responsibilities.

### 3.3 Invalid Distinctions

- Do not define streaming as only `input URL -> play`.
- Do not turn protocol names into the top-level product experience.
- Do not split queue, favorites, or DSP attachment points by source type when a shared abstraction can carry the behavior.
- Do not force live radio or continuous streams into `Album` or `Track` semantics when the source is not actually track-structured.
- Do not leak tokens, cookies, certificate exceptions, or share-path handling into generic page state without an owning connection model.

## 4. Normative Product Definition

`LoFiBox Zero` streaming is the capability that extends the product from a local-file player into a network audio terminal and media accessor.

A streaming-capable Linux build must cover three product responsibilities:

1. play remote audio content
2. access remote media systems and libraries
3. manage network-aware playback lifecycle, metadata, cache, and control

This extension changes capability, not product identity.
The product must remain offline-capable and must not require network access for core local playback.

## 5. Unified Media Model

### 5.1 Shared Abstraction Rule

Upper layers must not branch first on protocol.
They must branch first on product meaning.

The shared app layer should converge local and remote playback through unified abstractions such as:

- `PlayableItem`
- `Track`
- `Station`
- `Album`
- `Artist`
- `Playlist`
- `ResolvedStream`
- `PlaybackSession`

### 5.2 `Track` Versus `Station`

- `Track` is the canonical object for on-demand, seekable, library-addressable audio.
- `Station` is the canonical object for live or long-running channel-style playback whose now-playing metadata may change over time.
- A source may expose only `Station` objects, only `Track` objects, or both.
- `Album`, `Artist`, and normal library browse hierarchies apply only when the source actually has catalog structure.

### 5.3 Cross-Source Consistency

When a source can supply the needed semantics, the product should support:

- one shared playback queue
- one shared search entry point
- one shared favorite/history model
- one shared metadata and cover-art presentation
- one shared EQ/DSP/analysis entry path

When a source cannot support the same semantics, the UI should degrade explicitly rather than pretending the source behaves like local files.

## 6. Capability Surface

### 6.1 Remote Source Access

The full streaming domain includes these source classes:

- direct audio URLs over `HTTP` and `HTTPS`
- remote audio files such as `MP3`, `AAC`, `FLAC`, `OGG`, `Opus`, and remote `WAV` or `PCM` streams
- playlist entry formats such as `M3U`, `M3U8`, `PLS`, and `XSPF`
- broadcast and internet-radio style sources such as `Icecast`, `Shoutcast`, and other long-lived audio streams
- adaptive streaming formats such as `HLS` and `DASH`
- LAN or remote-share media roots exposed through `SMB`, `NFS`, `WebDAV`, `FTP`, or `SFTP`

These are different source classes even when they all end in audible playback.

### 6.2 Media Server Integration

The streaming domain should support integration with server-based libraries, especially:

- `Jellyfin`
- `Navidrome`
- `Emby`
- `Subsonic` and `OpenSubsonic` compatible servers
- `DLNA` and `UPnP` media servers

Server integration must include a connection model rather than ad-hoc settings fields.
That model should support:

- adding and editing servers
- testing connectivity
- authenticating and restoring sessions
- switching among multiple saved servers
- selecting a default server
- distinguishing read-only versus writable capabilities when the source exposes them

The small-screen Settings flow must present this connection model in two levels:

- first, a Remote Setup page listing every supported remote source kind, including server providers, direct URL, radio, playlist manifests, adaptive streams, LAN shares, and DLNA/UPnP
- second, a source-kind-specific profile/settings page for address, username, credential reference or secret entry, TLS policy, permission status, connectivity test, and save behavior

Settings must not expose separate top-level rows such as server login, credential, TLS, or permission fields before the user chooses a remote source kind.

Persisted server connection profiles and saved-source rules belong to `docs/specification/lofibox-zero-persistence-spec.md`.

### 6.2.1 First-Batch Real Provider Requirements

The first-batch server providers are not allowed to remain manifest-only placeholders.
For each provider family marked supported by `RemoteSourceRegistry`, the implementation must expose at least:

- authenticated connection probing
- library-track enumeration
- title/artist/album/duration mapping into the unified `RemoteTrack` model
- search where the server API supports it
- recent or latest track lookup where the server API supports it
- stream URL resolution into a `ResolvedRemoteStream`
- mock protocol tests that do not require a real user server
- an optional real-server smoke test that reads credentials only from environment variables and skips when unset

`EmbySource` is a first-batch real provider.
It must use Emby-compatible authentication, catalog, latest-item, search, and audio-stream endpoints rather than being represented only by a capability row in Source Manager.
It must only project server items that are audio catalog items or that explicitly expose audio-only media streams into `RemoteTrack`.
Movie, episode, series, trailer, music-video, and generic video objects are not valid `RemoteTrack` values, even when the server exposes an endpoint that could extract or transcode an audio stream from them.
When no audio result exists, search and browse must return an empty or degraded result rather than falling back to video items.
Real Emby credentials must never be committed, persisted in the profile store, printed in logs, or embedded in tests.

### 6.3 Catalog Browse And Search

When the source exposes catalog structure, the product should be able to browse:

- artists
- albums
- tracks
- genres
- favorites
- recently added
- recently played
- playlists
- folder views

Search should be able to cover:

- track title
- artist
- album
- playlist
- station or channel
- folder or path keywords

When local and remote search are merged, results should remain grouped or otherwise explain their source.

### 6.3.1 Remote Provider Data Contract

App-facing code must consume remote media through normalized provider output, not raw server protocol fields.
Provider scripts own protocol translation.
They must convert Jellyfin, Emby, OpenSubsonic/Navidrome, DLNA, share, URL, radio, and future provider responses into the shared app contract before `AppRuntimeContext`, playback, search, queue, lyrics, or page projection code sees the data.

The normalized remote track contract must carry:

- stable remote item id
- title
- artist
- album
- album id when known
- duration when known
- source id
- source label for UI display, such as `Emby`, `Jellyfin`, or a saved profile name
- artwork key or non-secret artwork URL when the provider exposes one
- local-cacheable lyrics fields when the provider exposes lyrics
- local-cacheable audio fingerprint or recording identity hint when the provider exposes one

The normalized remote catalog-node contract must carry the same metadata where applicable, plus:

- node kind, such as artists, artist, albums, album, playlists, folders, favorites, recently-added, recently-played, stations, or tracks
- title and subtitle
- playable flag
- browsable flag

Application code may cache, display, queue, and resolve these normalized objects.
It must not parse server-specific field names such as Jellyfin `RunTimeTicks`, Emby media-source structures, OpenSubsonic `song`, or provider-specific image tags outside provider/tooling code.

Remote Now Playing parity follows the same rule:

- title, artist, album, duration, lyrics, artwork placeholder/cache behavior, visualization, play/pause, seekability, and DSP attachment must be driven by the shared playback session
- the only required visible difference from local playback is source attribution, shown as the remote source label in the Now Playing top bar
- raw stream URLs and credentials must stay out of Now Playing and may only appear redacted in Stream Detail or diagnostics pages

### 6.4 Playback And Buffering

Streaming playback is not complete unless it handles network behavior explicitly.

The playback system should support:

- initial buffering
- a minimum playable buffer threshold
- visible buffer state
- pause or wait behavior when the buffer falls below a safe threshold
- sequential read while playing
- next-item prefetch
- playlist or album prefetch when continuity helps
- automatic retry after transient network failures
- reconnection after server timeouts or broken radio streams
- quality selection for adaptive or server-transcoded sources
- choosing between original files and transcoded streams when the source offers both

### 6.5 Mixed Queue And Control Semantics

The player should be able to mix source types inside one queue, including combinations of:

- local files
- server-backed tracks
- remote URL tracks
- saved radio or station entries

Streaming-aware control should include source-specific capabilities where relevant, such as:

- refresh stream
- reconnect
- choose quality or bitrate
- show network condition
- show cache or buffer remaining
- show `Live` status for live sources

### 6.6 Metadata And Explainability

Streaming is not just audible output.
The product should surface enough information for the content to stay understandable.

Remote playback should support metadata such as:

- title
- artist
- album
- track number
- duration when known
- cover art
- bitrate
- codec or format
- sample rate
- channel count

Live or radio sources may additionally expose:

- station or channel name
- current program name
- live now-playing title
- stream label that updates while playback continues

Metadata precedence should prefer richer source truth over weaker fallbacks.
For example, server metadata should beat filename heuristics when both exist.

A detail or debug view for a remote item should be able to explain:

- the source or server name
- whether the item is local, remote, on-demand, or live
- the resolved playback format or quality
- current buffer state
- connection status

If a raw playback URL contains credentials or tokens, it must be redacted or summarized rather than shown verbatim.

### 6.7 Cache And Offline Behavior

Streaming quality depends heavily on cache behavior.

The capability surface should include:

- transient playback buffer
- recent-item cache with size-based and time-based eviction
- persistent cover-art cache
- persistent metadata cache
- cached server directory or browse results where useful
- cached station or channel lists

Optional higher-level behavior may include:

- saving remote tracks for offline playback
- offline syncing of chosen albums or playlists
- metadata-only caching without audio download

Offline save is source-dependent and should respect source permissions, authentication state, and storage policy.
Implementation ownership is locked to a unified `CacheManager` boundary.
The manager owns cache buckets for playback buffers, recent browse items, artwork, metadata, lyrics, remote directory catalogs, station lists, and offline audio.
Each bucket exposes capacity policy, age policy, usage reporting, and garbage collection rather than scattering expiration logic inside protocol clients or UI pages.
Remote directory caches, station-list caches, recent-browse caches, offline track saves, album offline sync plans, and playlist offline sync plans are part of this same cache/offline domain.
Host implementations must place runtime cache material under the XDG cache root and must not write offline or cache data into the installation tree.

Read-only remote libraries are not writeback failures.
If a remote provider cannot mutate server-side track tags, or if the server item lacks metadata that LoFiBox later obtains from a trusted catalog, LoFiBox must persist the accepted metadata locally under the metadata cache bucket and match it back by stable remote source identity plus item id on later playback.
Lyrics accepted for a read-only remote item must be persisted locally under the lyrics/cache domain rather than discarded merely because the server item cannot be tagged.
When an audio fingerprint can be obtained for a remote item, LoFiBox must keep it in the persistent local fingerprint index or equivalent stable metadata cache so future identity, lyrics, and artwork decisions can be made without server-side writeback authority.
This local metadata, lyrics, and fingerprint cache is a display/playback continuity fact, not a server truth override, and must not be written into the remote server unless the provider explicitly declares writable metadata authority.

### 6.8 Accounts, Authentication, And Security

The streaming domain should support the connection patterns commonly needed by remote sources, including:

- username and password
- token
- API key
- session or cookie-based auth

Security handling should include:

- `HTTPS`
- certificate validation
- explicit handling of self-signed or custom-certificate environments
- clear failure states when a secure connection cannot be established

Credential material must not be treated like ordinary media metadata.
It needs dedicated storage and lifecycle handling as defined in `lofibox-zero-credential-spec.md`.

## 7. Delivery Tiers

These tiers help sequence delivery without shrinking the capability boundary.

### 7.1 Core Streaming Capability

- direct remote audio playback
- playlist manifest support
- internet radio support
- media server connection and browse
- LAN share access
- buffering and automatic reconnect
- shared queue integration
- remote metadata and cover-art display

### 7.2 Enhanced Streaming Capability

- multiple saved servers
- unified local plus remote search
- richer cache policies
- favorites and recent-playback sync when the source supports it
- adaptive bitrate or transcode policy controls
- visible network-awareness signals

### 7.3 Advanced Streaming Capability

- multi-source unified media library views
- offline-save and sync workflows
- full DSP/EQ/analysis parity for streamed content
- local and remote media governance through one product model

## 8. Architecture Contract

The capability should be decomposed by responsibility, not by screen or protocol.

### 8.1 Suggested Responsibility Boundaries

- `SourceRegistry`
  Owns saved servers, shares, stations, credential references, default-source rules, and capability flags.
- `SourceProvider`
  Knows how to open or enumerate a specific source family.
- `CatalogProvider`
  Browses and searches media structure when the source exposes a catalog.
- `StreamResolver`
  Turns a `PlayableItem` into one or more concrete playable stream candidates or `TrackSource` inputs, including manifest resolution, auth-bearing URLs, and server transcode endpoints.
- `PlaybackPipeline`
  Owns buffering, read-ahead, retry or reconnect, handoff into the shared media pipeline, and playback-session state.
- `MetadataProvider`
  Merges file tags, server metadata, station updates, and cached artwork.
- `CacheManager`
  Owns transient buffer cache, metadata cache, artwork cache, and offline-save storage policy.

The detailed lower-level decode, metadata, DSP-chain, and sink behavior for a resolved `TrackSource` belongs to `docs/specification/lofibox-zero-media-pipeline-spec.md` and `docs/specification/lofibox-zero-audio-dsp-spec.md`.
Shared app-level projection of saved sources, active source, and default source belongs to `docs/specification/lofibox-zero-app-state-spec.md`.

### 8.2 Layering Rule

Shared app code may depend on stable media and playback abstractions.
It must not depend directly on server-specific SDK objects, raw Linux device paths, or protocol parser details.

Platform code may own Linux transport and device integration details, but the product meaning of a `Track`, `Station`, queue item, or playback state must stay out of protocol glue.

### 8.3 Capability Flags

Remote items and sources should advertise capability facts rather than forcing pages to guess them.
Useful flags include whether an item is:

- live or on-demand
- seekable or non-seekable
- cacheable or non-cacheable
- offline-eligible or stream-only
- read-only or server-write-capable

## 9. Page-Spec Boundary

This document makes streaming a valid product capability.
It does not, by itself, define the final page map.

Before implementing streaming-specific pages or inserting streaming controls into existing pages, the page spec must be updated with explicit screen contracts for things such as:

- source management
- server login and connection status
- remote catalog browse and search flows
- station lists
- streaming detail or diagnostics views

Until that page-level work exists, do not smuggle partial remote-source UI into unrelated current implementation pages.

## 10. AI Constraints For Future Work

- Do not regress streaming back into a one-off URL player after this capability boundary has been established.
- Do not fork playback, queue, or DSP semantics by source type unless a real product distinction requires it.
- Do not flatten live streams and on-demand tracks into one fake metadata model.
- Do not let auth, cache, or protocol parsing details become the shared app's top-level concepts.
- If later work changes the meaning of `PlayableItem`, `Track`, `Station`, or the provider boundaries above, update this specification before changing code structure.

## 22. Current Implementation Convergence

As of 2026-04-27, the implementation baseline must treat the following as concrete product-domain objects, not page-local conveniences:

- `RemoteSource` families include direct URL, internet radio, playlist manifest, HLS, DASH, SMB, NFS, WebDAV, FTP, SFTP, DLNA/UPnP, Jellyfin, OpenSubsonic, Navidrome-compatible, and Emby.
- `RemoteCatalogNode` is the shared browse model for artists, albums, tracks, genres, playlists, folders, favorites, recently added, recently played, and stations.
- `StreamSourceClassifier` owns URI/protocol recognition for direct files, adaptive manifests, playlist manifests, radio-style streams, LAN shares, and discovery URLs.
- `StreamingPlaybackPolicy` owns buffer thresholds, reconnect/retry plans, radio reconnection behavior, next-item and album/playlist prefetch, quality preference, transcoding preference, and stream diagnostics.
- `RemoteStreamDiagnostics` is the stream-detail truth for source identity, redacted effective URL, protocol, bitrate, codec, buffer state, and connection state.

UI pages, provider adapters, and playback orchestration must consume these shared models instead of inventing per-page or per-protocol representations.
