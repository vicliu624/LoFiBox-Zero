<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Media Pipeline Specification

## 1. Purpose

This document defines the media-format and playback-pipeline capability for `LoFiBox Zero`.

It exists to stop future work from treating `supports more formats` as a pile of one-off file-extension branches.
In this project, format support is not a UI feature and not a per-codec special case.
It is the lower-level capability that lets `LoFiBox Zero`:

- recognize a playable media source
- extract or decode audio from that source
- read metadata and artwork
- hand decoded audio into a shared DSP path
- hand processed audio to the platform output path

Use this document when deciding how local files, remote files, and resolved network streams are allowed to converge inside one playback path.

For final product meaning, use `lofibox-zero-final-product-spec.md`.
For architecture placement, use `project-architecture-spec.md`.
For audio fingerprinting and recording identity, use `lofibox-zero-track-identity-spec.md`.
For EQ, loudness, `ReplayGain`, limiter, and other DSP behavior between decode and output, use `lofibox-zero-audio-dsp-spec.md`.
For remote-source and server behavior, use `lofibox-zero-streaming-spec.md`.
For product intent and carry-forward behavior, use `legacy-lofibox-product-guidance.md`.

## 2. Scope

This document covers:

- media-format capability as a pipeline concern rather than a page feature
- source, decode, metadata, DSP-handoff, and output responsibility boundaries
- broad Linux format support goals
- backend abstraction rules for Linux media stacks
- cross-source convergence for local and remote audio items

This document does not define:

- final UI pages for format inspection
- a mandatory choice between specific backend libraries
- audio-device settings UX
- transport or account behavior for remote servers

## 3. Distinction Contract

### 3.1 Current Confusions

- `format support` is often confused with `file extension support`.
- `codec`, `container`, `metadata`, and `audio output` are often flattened into one fake `play file` responsibility.
- local files, remote files, and resolved network streams may arrive through different source families, but they should not automatically fork the decode path.
- a media backend such as `FFmpeg` or `GStreamer` is an implementation family, not a shared app concept.

### 3.2 Valid Distinctions

- `TrackSource` answers where bytes come from.
- `Decoder` answers how audio streams are recognized, selected, and decoded into playable audio frames.
- `MetadataProvider` answers how tags, artwork, and stream descriptors are read and normalized.
- `TrackIdentityProvider` answers which real-world recording a source represents.
- `DspChain` answers how decoded PCM is transformed between decode and output.
- `AudioSink` answers how decoded audio is handed to the platform output layer.
- `streaming`, `local files`, and `LAN shares` are source concerns; decode and sink concerns begin after a playable source has been resolved.

### 3.3 Invalid Distinctions

- Do not model format support as `if mp3`, `if flac`, `if aac` branches scattered through app logic.
- Do not treat filename extensions as the final authority on container or codec identity.
- Do not duplicate metadata parsing rules by source type when one normalized metadata path can serve both local and remote tracks.
- Do not leak backend-specific classes or graph objects into shared app code.

## 4. Normative Product Definition

`LoFiBox Zero` media-format support is a pipeline capability, not a page feature.

A Linux-capable build should treat broad audio support as the result of a unified media pipeline that can:

1. accept a resolved `TrackSource`
2. recognize or probe the playable stream structure
3. decode audio into a common playback representation
4. extract metadata and artwork
5. pass decoded audio through a `DspChain` and volume path
6. deliver processed audio to an output sink

This definition applies to:

- local files
- resolved remote files
- resolved network streams when they behave like track playback

Live stations or continuously updating streams may still differ in metadata and seek behavior, but they should reuse the same lower-level pipeline where their media characteristics allow it.

## 5. Linux Format Capability

### 5.1 Broadened Target Set

The Linux media capability should broaden beyond the legacy `MP3` plus `WAV` starting point.

The target playback set should include:

- `MP3`
- `AAC`
- `WAV` and `PCM`
- `FLAC`
- `Ogg Vorbis`
- `Opus`
- `ALAC`
- `APE`
- `AIFF`

This is a target capability for the Linux media stack, not permission to hardcode format-specific branches throughout the app.

### 5.2 Format Support Must Be Backend-Driven

The product should realize this capability through a Linux media backend layer rather than one-off decoders wired directly into app code.

Candidate backend families include capability-oriented media stacks such as:

- `FFmpeg`
- `GStreamer`

This specification does not require a single library choice today.
It requires that whichever backend is selected must sit behind our own abstractions.

### 5.3 Capability Is More Than Decode

`Supports format X` is only meaningful when the build can answer all of these questions coherently:

- can the source be opened?
- can the stream structure be recognized?
- can playable audio be decoded?
- can metadata be read or normalized?
- can decoded audio reach the platform sink?

A build may partially support a format for decode while having weaker metadata coverage.
That difference must be treated as a capability fact, not hidden by vague marketing language.

## 6. Pipeline Model

### 6.1 `TrackSource`

`TrackSource` is the normalized input to the playback pipeline.

It represents a playable source after source resolution has already happened.
It may come from:

- a local filesystem path
- a LAN-mounted or share-backed file
- a remote URL that behaves like a file or stream
- a server-resolved playback endpoint

`TrackSource` owns source identity, byte-access method, and source-level hints.
It must not own UI state or output-device concerns.

### 6.2 `Decoder`

`Decoder` owns media recognition and audio decode.

Its responsibility includes:

- container or stream recognition
- stream selection and demux responsibility where needed
- codec decode into a common playback representation
- surfacing technical playback facts such as codec, sample rate, channels, and duration when known

This is the place where format diversity belongs.
The rest of the app should not care whether the input was `MP3`, `FLAC`, `AAC`, or `Opus`.

### 6.3 `MetadataProvider`

`MetadataProvider` owns metadata extraction and normalization.

It should be able to surface:

- title
- artist
- album
- track number
- duration when available
- embedded artwork when available
- technical fields such as bitrate, codec label, sample rate, and channels

It should support both:

- metadata carried by local or remote files
- metadata provided by server or streaming context

When multiple metadata sources exist, richer source truth should beat weaker filename heuristics.

### 6.3.1 `TrackIdentityProvider`

`TrackIdentityProvider` owns recording identity resolution.

It may use:

- embedded metadata as a seed
- filename metadata as a seed
- audio fingerprints as primary evidence
- MusicBrainz identifiers as stable recording, release, and release-group identity

It must not be collapsed into `LyricsProvider`, `ArtworkProvider`, or `TagWriter`.
Those providers may consume accepted identity, but they must not independently decide which recording a file represents.

### 6.4 `DspChain`

`DspChain` owns ordered audio processing after decode and before output.

It is the insertion point for processors such as:

- `EQ`
- `loudness`
- `ReplayGain`
- `limiter`
- future corrective or analytical filters

The detailed behavior of those processors belongs to `docs/specification/lofibox-zero-audio-dsp-spec.md`.

### 6.5 `AudioSink`

`AudioSink` owns handoff of decoded audio to the Linux output layer.

Its responsibility includes:

- accepting a common decoded audio representation
- adapting to the active platform output path
- reporting output-level failures to the playback layer

It must not own source discovery, metadata interpretation, or queue policy.

### 6.5.1 Output Stability

The playback path must protect the listening experience at track boundaries.

When decoded PCM is handed to the platform output layer, the implementation must:

- start output only after a small prebuffer exists, or after end-of-stream for very short audio
- keep the output stream paused while prebuffering so playback does not begin with an underrun
- avoid destroying, pausing, resuming, or querying the output stream from multiple threads without a single ownership or locking rule
- expose visualization data only after real audio output has started, not merely after decode has produced the first samples
- advance the user-visible playback clock only after the output sink has confirmed that processed PCM is entering the platform playback path
- drain queued audio at end-of-stream before declaring the backend finished

The playback clock and spectrum are projections of audible playback.
They must not use decoder start, remote stream resolution, metadata enrichment, analyzer startup, or a successful pipe write to an intermediate helper as the truth that sound has begun.
If the implementation uses external Linux helpers, those helpers remain `AudioSink` adapters; they must not define a separate product playback model.
On Debian targets, the implementation may prefer a lower-latency sink such as
PipeWire `pw-cat` or ALSA `aplay` over `ffplay` when decoded PCM is already
owned by LoFiBox, because the product truth is
`Decode -> DspChain -> AudioSink`, not the identity of the helper executable.
When a PipeWire runtime socket is available, `pw-cat` is the preferred desktop
sink because it cooperates with the active user audio server across rapid
track restarts; `aplay` remains the Linux-first fallback for lean ALSA-only
targets.
External decode/sink helpers must be launched with a multithread-safe process
creation path such as `posix_spawn` on Linux. Runtime socket commands may arrive
on server threads while the presenter thread is rendering, so helper startup
must not rely on fork-child C++ allocation or other post-fork work that is unsafe
inside a multithreaded process.

The shared playback controller must consume backend completion as a discrete event.
It must not poll and act on the same backend completion state multiple times in one UI update, because that can create start or end jitter when the audio thread, UI thread, and queue-advance logic cross at a track boundary.

### 6.6 Shared Playback Path

Once a playable source has become a `TrackSource`, local and remote media should converge through the same decode, DSP, and sink path whenever their behavior allows it.

This means:

- a local file should not have a special decode path purely because it is local
- a remote file should not have a special decode path purely because it came from a network source
- resolved streaming items should reuse the same lower-level media pipeline when they present normal track-like audio streams

## 7. Capability Manifest And Reporting

The shipped build should treat media capability as an explicit manifest, not as hidden implementation folklore.

The implementation should be able to answer at least these questions for a format or stream family:

- playable or not playable
- metadata coverage level
- artwork coverage level
- seekability
- live versus on-demand behavior

This supports better diagnostics, clearer feature gating, and safer page behavior later.

## 8. Relationship To Audio DSP And Streaming

This document is lower-level than the streaming spec and adjacent to the audio-DSP spec.

- `lofibox-zero-audio-dsp-spec.md` decides what belongs inside `DspChain` after decode.
- `lofibox-zero-streaming-spec.md` decides how remote sources, media servers, playlists, and stations enter the system.
- this document decides how a resolved playable source is interpreted, decoded, handed into DSP, described, and output

In other words:

- streaming resolves *what to play*
- the media pipeline resolves *how to parse, decode, hand into DSP, describe, and output it*
- the audio-DSP spec resolves *what happens inside the processing chain after decode*

## 9. Architecture Contract

### 9.1 Shared App Boundary

Shared app code may depend on stable concepts such as:

- `TrackSource`
- `Decoder` capability results
- `DspChain` capability boundaries
- normalized metadata
- playback-session facts

Shared app code must not depend directly on:

- `FFmpeg` objects
- `GStreamer` graph or element types
- Linux audio-device handles
- backend-specific parser details

### 9.2 Adapter Boundary

Backend-specific media logic belongs behind an adapter layer.

That adapter layer may translate backend capabilities into our own:

- decode results
- metadata structures
- capability facts
- sink errors

### 9.3 Output Boundary

`AudioSink` belongs to the playback/output side of the platform boundary.
It may integrate with Linux-specific audio infrastructure, but the shared app must only see normalized playback success, failure, and format facts.

## 10. AI Constraints For Future Work

- Do not implement broad format support as extension-based branching in app code.
- Do not duplicate one metadata parser per source family unless the source truly exposes different metadata semantics.
- Do not bypass `TrackSource` just because one early backend makes local files easy.
- Do not bypass `DspChain` just because one early implementation only supports simple playback.
- Do not let backend-selection convenience define the shared product model.
- If later work changes the meaning of `TrackSource`, `Decoder`, `MetadataProvider`, `DspChain`, or `AudioSink`, update this specification before changing code structure.
