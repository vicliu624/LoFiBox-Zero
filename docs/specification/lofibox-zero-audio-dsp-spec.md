<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Audio DSP Specification

## 1. Purpose

This document defines the audio DSP and equalizer capability domain for `LoFiBox Zero`.

It exists to stop future work from treating `EQ` as only a row of sliders on one page.
In this project, a mature EQ capability is not just visible band controls.
It is the audio-processing domain that shapes, protects, normalizes, and adapts playback after decode and before output.

Use this document when deciding how `EQ`, `preamp`, `bypass`, `loudness`, `ReplayGain`, `limiter`, and future audio processors belong in the product and where their responsibilities stop.

For final product meaning, use `lofibox-zero-final-product-spec.md`.
For architecture placement, use `project-architecture-spec.md`.
For decode, metadata, and output-pipeline behavior, use `lofibox-zero-media-pipeline-spec.md`.
For remote-source and server behavior, use `lofibox-zero-streaming-spec.md`.
For page-level UI constraints, use `lofibox-zero-page-spec.md`.

## 2. Scope

This document covers:

- equalizer capability beyond the current UI implementation page
- DSP-chain role and placement in the playback path
- preset systems and runtime state
- safety and consistency controls such as `preamp`, `ReplayGain`, and `limiter`
- advanced filtering and profile binding
- runtime update behavior, persistence, and system experience constraints

This document does not define:

- the exact DSP math or filter implementation
- a mandatory audio backend or library
- the final page map for advanced or professional EQ modes
- Linux audio-device administration UX

## 3. Distinction Contract

### 3.1 Current Confusions

- `EQ page` and `audio DSP domain` are not the same thing.
- `graphic EQ`, `loudness`, `ReplayGain`, and `limiter` are related, but they are not one control.
- `preset persistence`, `current runtime state`, and `per-device binding` are different responsibilities.
- `realtime adjustment` must not be confused with `restart the playback chain`.

### 3.2 Valid Distinctions

- `DspChain` is the ordered processing path between decode and output.
- `EqProfile` is a persisted configuration object, not the processing engine itself.
- `EqSessionState` is the runtime truth about what is currently active.
- `EqEngine` applies EQ-related processing to decoded PCM.
- `EqManager` owns active profile selection, runtime toggles, and binding decisions.
- `PresetRepository` stores built-in and user-authored DSP presets.
- `OutputDeviceBinding` maps a physical or logical output target to a DSP profile choice.

### 3.3 Invalid Distinctions

- Do not hardwire EQ as a special branch inside the player UI.
- Do not rebuild or restart track playback merely to apply slider changes.
- Do not assume one global EQ profile fits every output device or content type.
- Do not treat protective processing such as `preamp` or `limiter` as optional decoration when the product allows aggressive gain boosts.

## 4. Normative Product Definition

`LoFiBox Zero` EQ is a first-class DSP capability, not a settings afterthought.

A mature EQ and DSP capability should cover five layers:

1. basic equalization
2. preset management
3. advanced audio controls
4. advanced EQ and filter capability
5. system-level runtime experience

This capability belongs inside the audio-processing path.
It must not be modeled as a separate player branch or a page-only effect.

## 5. Capability Layers

### 5.1 Basic Equalization

A mature graphic EQ capability should grow beyond the current six-band implementation page.

The recommended target is:

- `10-band` graphic EQ
- or `15-band` graphic EQ

Common band sets may include frequencies such as:

- `31 Hz`
- `62 Hz`
- `125 Hz`
- `250 Hz`
- `500 Hz`
- `1 kHz`
- `2 kHz`
- `4 kHz`
- `8 kHz`
- `16 kHz`

Each band should support:

- positive and negative `dB` gain adjustment
- reset to `0 dB`
- clear enabled or neutral state semantics

The mature domain should also include:

- `preamp`
- global `EQ` enable
- `bypass`
- `A/B` comparison between processed and unprocessed sound

The current UI implementation remains governed by `lofibox-zero-page-spec.md`.
This document defines the broader capability boundary beyond today's implemented page surface.

### 5.2 Preset System

An EQ capability is incomplete if only expert users can benefit from it.

The preset system should include built-in presets such as:

- `Flat`
- `Bass Boost`
- `Treble Boost`
- `Vocal`
- `Rock`
- `Pop`
- `Jazz`
- `Classical`
- `Electronic`
- `Podcast` or `Speech`

User preset behavior should support:

- create preset
- copy preset
- rename preset
- delete preset
- overwrite or save current settings

Preset switching during playback should support:

- immediate effect
- no playback interruption
- no fake `Apply` step that rebuilds the whole playback session

### 5.3 Advanced Audio Controls

A more complete DSP capability should include controls adjacent to EQ in the same processing chain, especially:

- left or right `balance`
- `loudness` with adjustable strength
- `ReplayGain` with track mode, album mode, and target loudness behavior
- `limiter` or peak protection

These are not identical to graphic EQ, but they belong to the same runtime DSP domain.

### 5.4 Advanced EQ And Filter Capability

For advanced users, the DSP domain may grow to include:

- `parametric EQ`
- configurable center frequency
- configurable gain
- configurable `Q` or bandwidth
- `high-pass` filters
- `low-pass` filters
- per-output-device DSP profiles
- per-content-type DSP bindings such as music, speech, radio, movie, or accompaniment

These are valid directions for `LoFiBox Zero`, but they do not automatically rewrite the current UI implementation contract.

### 5.5 System-Level Runtime Experience

The product should treat DSP as a realtime experience, not an offline settings editor.

The system-level experience should include:

- realtime audible updates while parameters change
- minimized click, pop, or abrupt jumps during preset or state changes
- frequency-response or curve visualization when advanced UI exists
- processed versus bypass comparison feedback
- peak-level or clipping feedback when protective UX is available
- persistence and auto-restore for active state, presets, and device bindings

## 6. User-Facing Modes

The product may present DSP in multiple layers of complexity.

### 6.1 Simple Mode

The approachable default may include:

- `EQ` enable
- preset selector
- `preamp`
- graphic EQ sliders
- reset action

### 6.2 Advanced Mode

A more detailed mode may include:

- frequency-response curve
- `loudness`
- `balance`
- `limiter`
- `ReplayGain`
- save-as-preset behavior

### 6.3 Professional Mode

A high-control mode may include:

- `parametric EQ`
- per-band `frequency`, `Q`, and `gain`
- `high-pass` and `low-pass`
- output-device binding
- configuration import and export

Before any of these expanded modes become real screens, the page and layout specs must be updated explicitly.

## 7. DSP Chain Model

### 7.1 Core Processing Path

The playback path should not stop at:

`Source -> Decode -> Output`

The mature processing path should be understood as:

`Source -> Decode -> DspChain -> Volume -> Output`

`TrackSource`, `Decoder`, and `AudioSink` belong to `lofibox-zero-media-pipeline-spec.md`.
This document defines what belongs inside `DspChain`.

### 7.2 `DspChain`

`DspChain` is the host for ordered processors such as:

- `EQ`
- `loudness`
- `ReplayGain`
- `limiter`
- future corrective or analytical filters

`EQ` is one important node in this chain, not the whole chain itself.

### 7.3 Runtime Flow

A correct runtime flow looks like this:

1. decoder outputs PCM
2. playback pipeline checks active DSP state
3. PCM passes through the active `DspChain`
4. processed PCM proceeds through volume and output
5. parameter changes update DSP state without restarting playback
6. later audio frames are processed with the updated state immediately

This is a hot-update model, not a replay model.

### 7.4 Runtime Safety And Smoothness

The DSP chain should account for:

- clipping risk when multiple EQ bands are boosted
- `preamp` or equivalent gain headroom control
- `limiter` or explicit warning paths
- parameter smoothing or interpolation
- minimization of pop or abrupt artifacts
- CPU cost on smaller Linux devices
- long-running playback stability

## 8. Data And Responsibility Model

### 8.1 `EqBand`

`EqBand` represents one controllable band and may include fields such as:

- `frequency`
- `gain_db`
- `q`
- `enabled`

### 8.2 `EqProfile`

`EqProfile` represents a complete stored DSP or EQ configuration and may include fields such as:

- `id`
- `name`
- `type`
- `preamp_db`
- `bands[]`
- `balance`
- `loudness_enabled`
- `limiter_enabled`
- `replaygain_mode`
- `is_default`

### 8.3 `OutputDeviceBinding`

`OutputDeviceBinding` represents a mapping such as:

- `output_device_id`
- `eq_profile_id`

### 8.4 `EqSessionState`

`EqSessionState` represents runtime truth and may include:

- `current_profile_id`
- `enabled`
- `bypass`
- `last_modified_time`

### 8.5 `EqEngine`

`EqEngine` is the processing executor.

Its job is to:

- initialize filters
- update filter parameters
- process PCM frames
- return processed PCM

### 8.6 `EqManager`

`EqManager` owns configuration and active-state coordination.

Its job is to:

- select the active profile
- bind profiles to outputs or contexts
- save and load state
- notify the playback path about hot updates

### 8.7 `PresetRepository`

`PresetRepository` owns persistence for:

- system presets
- user presets
- device-specific presets

The implemented DSP domain must expose these final-form controls through shared audio/DSP objects rather than through page-local state:

- built-in presets including `Flat`, `Bass Boost`, `Treble Boost`, `Vocal`, `Rock`, `Pop`, `Jazz`, `Classical`, `Electronic`, and `Podcast / Speech`
- user preset create, copy, rename, delete, overwrite, import, and export semantics
- output-device and content-type bindings
- 10-band graphic EQ plus parametric EQ bands
- high-pass and low-pass filter settings
- balance, loudness, limiter, ReplayGain mode, bypass, and preamp state
- smoothing helpers for non-abrupt parameter transitions

## 9. Relationship To Other Specifications

- `lofibox-zero-media-pipeline-spec.md` defines `TrackSource`, decode, metadata, `DspChain` placement, and output-sink boundaries.
- this document defines what belongs inside the DSP domain and how EQ-related runtime behavior should work.
- `lofibox-zero-streaming-spec.md` defines how remote sources enter the system; streamed content should use the same DSP entry path when its media behavior allows it.
- `lofibox-zero-page-spec.md` still defines the current implemented EQ page and does not automatically inherit the full mature DSP surface from this document.

## 10. AI Constraints For Future Work

- Do not collapse the DSP domain back into a page-local slider implementation.
- Do not model EQ changes as track restarts.
- Do not separate streamed and local DSP behavior unless a real media-semantic difference requires it.
- Do not let a single early six-band screen become the long-term architecture boundary for DSP.
- If later work changes the meaning of `DspChain`, `EqProfile`, `EqEngine`, `EqManager`, or `PresetRepository`, update this specification before changing code structure.

## 15. Current Implementation Convergence

As of 2026-04-27, the DSP baseline is a chain/profile domain rather than an EQ-page implementation detail:

- `DspChainProfile` carries graphic EQ, parametric EQ, high-pass/low-pass filters, loudness, balance, limiter, ReplayGain mode, preamp, bypass, smoothing, and clipping-protection intent.
- Preset governance includes built-in presets, user presets, import/export, duplication, rename, delete, and overwrite semantics.
- Device/content bindings map output devices and content categories to DSP profile ids without making the UI page own that decision.
- Playback stability policy owns gapless/crossfade preparation, transition lead time, and start/end jitter suppression as playback-chain behavior rather than visual behavior.
- The active compact EQ page state is converted into the playback `DspChainProfile`; slider and preset changes hot-update the running PCM DSP engine and must not restart the track.
- For the Debian host target, local and remote playback converge through a realtime PCM path: media is decoded to PCM, processed by the active `DspChain`, and then handed to the Linux output sink.

Future rendering pages may expose simple, advanced, or professional controls, but they must not redefine the DSP domain.
