<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Track Identity Specification

## 1. Purpose

This document defines how `LoFiBox Zero` decides which real-world recording a playable file or stream represents.

It exists to prevent metadata enrichment, artwork lookup, lyrics lookup, and tag writeback from each inventing their own private song-matching rules.
The product distinction is:

- `TrackIdentity` answers “which recording is this?”
- `MetadataProvider` answers “what descriptive fields can be shown or written?”
- `ArtworkProvider` answers “what image represents this track or release?”
- `LyricsProvider` answers “what lyrics belong to this identified recording?”
- `TagWriter` persists accepted facts; it does not decide truth.

For media decode and source handling, use `lofibox-zero-media-pipeline-spec.md`.
For persistence and repair rules, use `lofibox-zero-persistence-spec.md`.
For final product meaning, use `lofibox-zero-final-product-spec.md`.

## 2. Scope

This document covers:

- audio fingerprint generation
- fingerprint-backed recording lookup
- text metadata fallback
- confidence and conflict handling
- downstream enrichment use
- tag writeback authority

This document does not define:

- visual UI for identity inspection
- account management for third-party identity services
- full local fingerprint database implementation
- streaming-server catalog identity semantics beyond resolved playable items

## 3. Distinction Contract

### 3.1 Current Confusions

- A filename that looks correct is often confused with recording identity.
- Existing ID3/Vorbis tags are often treated as truth even when they may be stale or polluted by earlier enrichment.
- A lyrics API hit is often treated as proof that the lyrics belong to the current file.
- Tag writeback is often treated as a harmless cache update, even though it changes what other players will see.

### 3.2 Valid Distinctions

- `AudioFingerprintProvider` generates an audio-content fingerprint from decoded audio characteristics.
- `TrackIdentityProvider` resolves a fingerprint and/or metadata seed into a `TrackIdentity`.
- `TrackIdentity` may contain:
  - `recording_mbid`
  - `release_mbid`
  - `release_group_mbid`
  - normalized title, artist, album, and duration
  - confidence
  - source
  - whether the result was audio-fingerprint verified
- `MetadataProvider`, `ArtworkProvider`, and `LyricsProvider` may consume `TrackIdentity`; they must not redefine identity independently.

### 3.3 Invalid Distinctions

- Do not treat `LyricsProvider` as the component responsible for identifying a song.
- Do not treat `TagWriter` as the component responsible for deciding whether a match is correct.
- Do not treat title-only online matches as authoritative identity.
- Do not let one provider’s cache entry silently become durable truth without confidence and source context.

## 4. Normative Resolution Order

When network access is available and dependencies are configured, identity resolution should run in this order:

1. Generate an audio fingerprint using a local fingerprint provider such as `Chromaprint/fpcalc`.
2. Submit the fingerprint and duration to a fingerprint-backed identity service such as `AcoustID`.
3. Extract MusicBrainz recording, release, and release-group identifiers when present.
4. Use MusicBrainz text lookup only as a fallback when fingerprint identity is unavailable or misses.
5. Use filename and embedded metadata as seeds and consistency checks, not as final authority when fingerprint identity is strong.

If fingerprint identity and text metadata conflict:

- high-confidence fingerprint identity should win over filename and embedded metadata
- low-confidence fingerprint identity should be rejected or downgraded to non-authoritative
- the conflict must be logged

## 5. Downstream Enrichment Rules

### 5.1 Metadata

`MetadataProvider` should prefer accepted `TrackIdentity` fields when embedded metadata is missing, polluted, or inconsistent with the file path.

### 5.2 Artwork

`ArtworkProvider` should prefer `release_mbid`, then `release_group_mbid`, when looking up release artwork.

### 5.3 Lyrics

`LyricsProvider` should use accepted `TrackIdentity` title, artist, album, and duration as the primary lookup seed.

Lyrics providers must still validate their own returned candidates.
Fingerprint identity improves the query seed; it does not make an arbitrary lyrics response safe.

For remote media, lyrics lookup must use the stable remote media cache key (`remote-media-<kind>-<profile>-<item>`) rather than a synthetic display path as the durable lookup identity. The synthetic path may still be used as a filename-like query hint, but cache, miss invalidation, and accepted lyrics must attach to the stable source/item identity.

Remote catalog metadata and remote stream identity have different authority:

- remote catalog fields are source facts and good display/query seeds
- accepted high-confidence or fingerprint-verified `TrackIdentity` may govern the local display projection for title, artist, album, and duration
- weak online identity may fill only missing or clearly broken catalog fields
- read-only remote sources must receive local cache updates only; they must not be tag-written through the remote provider
- lyrics lookup must prefer the accepted remote `TrackIdentity` when available, then fall back to catalog title/artist/album/duration seeds

### 5.4 Enrichment End-To-End Observability

Metadata and lyrics enrichment are core product behavior, not best-effort visual
decoration. A release candidate is not acceptable if library rows, Now Playing,
GUI, TUI, and runtime snapshots remain dominated by `UNKNOWN` when usable local
tags, remote catalog facts, fingerprints, or online enrichment results exist.

The enrichment pipeline must keep these stages distinguishable in logs,
structured diagnostics, and tests:

- source facts: embedded tags, local library facts, remote catalog facts, and
  user-accepted local cache facts
- fingerprint facts: generated fingerprint, duration, lookup request, lookup
  hit/miss, and confidence
- online metadata facts: MusicBrainz/AcoustID or future provider request inputs,
  normalized candidate identity, confidence, and rejection reason
- lyrics facts: embedded lyrics, sidecar lyrics, local cache, online provider
  query terms, provider hit/miss, synced/plain distinction, and status message
- acceptance policy: whether a candidate is strong enough to govern display
  identity, only fill missing/broken facts, or be rejected
- persistence/cache: stable cache key used, read/write success, and freshness
- projection: runtime snapshot fields and GUI/TUI fields that consume the
  accepted identity and lyrics state

Remote media has the same observability requirement as local media. Remote
tracks must not be sent to metadata or lyrics providers with an empty filesystem
path as their effective identity. The stable remote cache key and the best
available governed title/artist/album facts must be the query seed. If a remote
server supplies weak or video-like catalog facts, the pipeline may improve the
local display projection from strong fingerprint or accepted metadata, but must
not write those facts back to the remote server.

Tests must include at least one local fixture and one remote fixture where:

- initial facts are incomplete or contain `UNKNOWN`
- enrichment supplies stronger identity and/or lyrics
- accepted results appear in runtime snapshot projection
- GUI/TUI/CLI-visible fields consume the projection rather than stale row text
- failed provider lookup produces a specific diagnostic reason instead of silent
  absence

### 5.5 Tag Writeback

Tag writeback may persist accepted identity fields so other players can benefit from the resolution.

When supported by the target file format, writeback should include:

- MusicBrainz recording ID
- MusicBrainz release ID
- MusicBrainz release-group ID
- identity source marker

Lyrics writeback should be allowed only after a provider result passes identity-aware matching rules.

## 6. Runtime And Persistence Rules

- Audio fingerprints are runtime derivations; they do not need to be permanently stored unless a later local fingerprint-index feature requires it.
- Accepted `TrackIdentity` may be cached as library metadata.
- Cached identity must include source, confidence, and fingerprint-verification state.
- Cached identity must be invalidatable by version when matching rules change.
- API keys or client keys for external identity services are credentials and must not be stored in ordinary cache records.
- `TrackIdentityProvider` `MUST NOT` directly overwrite shared `TrackMetadata`; `MetadataProvider` owns the decision to accept identity fields into display metadata or tag writeback.
- Online JSON string values `MUST` be decoded as real UTF-8 text before entering `TrackIdentity`, metadata cache, lyrics lookup, artwork lookup, or tag writeback.
- Literal escape artifacts such as `u6700u4f1a...` are invalid metadata and `MUST NOT` be written back to audio tags.
- Mojibake repair may be used as an adapter-layer repair for legacy ID3 text, but repaired display text and online lookup text must remain distinguishable from authoritative identity until accepted by the identity rules above.

## 7. Current Implementation Contract

The current host runtime implementation is allowed to provide this capability through:

- `ChromaprintFingerprintProvider`
- `AcoustIdIdentityClient`
- `TrackIdentityProvider`
- MusicBrainz fallback lookup

The app-facing contract is `TrackIdentityProvider`.
Platform-specific executable discovery, API-key environment variables, HTTP details, and JSON parsing belong inside the host adapter layer.

The current host implementation may use:

- `FPCALC_PATH` or `fpcalc`
- the built-in LoFiBox Zero AcoustID application client key
- `ACOUSTID_API_KEY` or `ACOUSTID_CLIENT_KEY` as explicit development or fork overrides

The AcoustID key is an application client key, not a per-user credential or write-capable user secret.
It may be shipped with the application so all users identify requests as LoFiBox Zero.

If fingerprint tooling or network access is unavailable, identity resolution must degrade to safe fallback behavior and log the reason.
