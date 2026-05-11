<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Pocket Groove Specification

## 1. Purpose

This document is the implementation baseline for Pocket Groove.

Pocket Groove is the device-side sampling groovebox mode for LoFiBox Zero. It is
not a page sketch and not a partial drum-machine feature. It defines a complete
creative loop that must remain explainable through LoFiBox Zero's existing
architecture boundaries.

The user-facing loop is:

```text
listen to a track
  -> capture a sample from the current track
  -> edit the sample
  -> assign it to one of 16 sound slots
  -> write a 16-step pattern
  -> arrange patterns with Song Chain
  -> perform with punch-in FX
  -> sync or trigger with MIDI
  -> export WAV
```

One-sentence product definition:

```text
Pocket Groove = device-side sampler + 16-step sequencer + Song Chain
              + Punch FX + MIDI + WAV export.
```

## 2. Authority And Relationship To Existing Specs

For source ownership and dependency direction, this spec follows
`project-architecture-spec.md`.

For product command boundaries, it follows
`application-command-boundary-spec.md`.

For audio DSP and creative effect distinctions, it follows
`lofibox-zero-audio-dsp-spec.md`.

For persistence paths, it follows `lofibox-zero-persistence-spec.md` and the XDG
rules in `project-architecture-spec.md`.

For current UI size constraints, it follows the shared `320x170` small-screen
profile.

This document adds the Pocket Groove-specific object model, mode behavior,
device UI, command model, audio render model, persistence model, extension
rules, and tests.

## 3. Product Definition

Pocket Groove is a creative mode inside the device experience. A user must be
able to complete one track idea on the device without WebUI, CLI, browser
editing, or a desktop DAW.

First-version required capabilities:

- enter Pocket Groove from the device
- pause the current player when entering
- capture from the current playing track
- capture from a local audio file
- use built-in samples
- trim sample start and end
- adjust sample gain and pitch
- normalize sample
- fade sample in and out
- reverse sample
- auto-slice sample
- manually define slices
- assign a slice to a step
- use 16 sound slots
- write 16 patterns
- use 16 tracks per pattern
- use 16 steps per pattern
- write step parameter locks
- arrange patterns with Song Chain
- perform with eight punch-in FX
- record punch-in FX to steps
- receive MIDI clock
- send MIDI clock
- respond to Start, Stop, and Continue
- trigger sound slots from MIDI notes
- map basic CC values to existing Groove commands
- save project
- load project
- rename project
- delete project
- auto-save the current project
- export current pattern as WAV
- export Song Chain as WAV

Pocket Groove is incomplete if it only toggles steps. It is also incomplete if
it can create a pattern but cannot export the result.

## 4. Non-Goals

Pocket Groove is not:

- a WebUI feature
- a WebSocket groove-state feature
- a CLI groove-control feature
- a browser pattern editor
- a remote arranging tool
- a desktop DAW
- a multitrack timeline
- a full-screen waveform editor
- a Pocket Operator clone
- an arbitrary plugin host

Explicitly forbidden first-version surfaces:

- no WebUI groove route
- no WebUI groove projection
- no WebSocket groove state sync
- no CLI command such as `lofibox groove ...`
- no browser pattern editing
- no remote groove control

Any later request to add one of those surfaces is a specification change and
must update this document before code is added.

## 5. Enduring Distinctions

### 5.1 Product Objects

The following are real product/domain objects:

- `GrooveProject`
- `GrooveSoundSlot`
- `SampleSlice`
- `GroovePattern`
- `GrooveTrack`
- `GrooveStep`
- `GrooveSongChain`
- `GrooveMidiSettings`
- `GrooveExportSettings`
- `PocketGrooveCommand`
- `GrooveRenderEngine`

The following are projections or adapters, not product truth:

- rendered overlay screens
- selected row indexes in overlays
- framebuffer geometry
- VNC/PocketFrame test button names
- JSON field order
- current implementation helper names
- CLI/WebUI concepts

### 5.2 UI And Domain

UI renders projection and sends commands. UI must not directly mutate
`GrooveProject`.

Valid flow:

```text
device input
  -> input router
  -> PocketGrooveCommand
  -> GrooveController / app groove bridge
  -> domain/audio/repository boundary
  -> projection
  -> UI render
```

Invalid flow:

```text
UI page
  -> mutate GrooveProject directly
  -> call decoder directly
  -> call WAV exporter directly
  -> write JSON directly
```

### 5.3 Audio And Export

Real-time playback and offline export must share `GrooveRenderEngine`. They may
have different schedulers and output sinks, but they must not have separate
sound algorithms.

The acceptance rule is:

```text
sound heard on device ~= sound exported to WAV
```

Differences caused by device output hardware are acceptable. Differences caused
by two divergent render engines are not.

First shipping implementation note:

- `Preview Render Playback` is allowed as an explicit interim device playback
  strategy: render the current pattern or Song Chain with `GrooveRenderEngine`
  into a cache WAV, then ask the existing audio backend to play that file.
- When this strategy is used, UI/status text must say preview/render playback
  and must not claim low-latency real-time groove playback.
- Step edits, sample edits, and FX locks must cause the next Play action to
  render a fresh preview.
- Punch FX UI must not say "hold to play" unless the platform input path has
  key-release events and the audio path applies FX to live audio blocks.
- The long-term target remains a real-time `GrooveSequencer -> SampleVoicePool
  -> GrooveMixer -> PunchFxProcessor -> AudioOutput` path sharing the same
  render semantics.

### 5.4 Effects And EQ

Punch-in FX are creative performance effects. They are not EQ presets and must
not use EQ preset cycling or the EQ preset repository.

EQ remains a playback DSP domain. Pocket Groove punch FX are groove performance
processors.

### 5.5 MIDI Boundary

MIDI input emits `PocketGrooveCommand` values. MIDI mapping must not call domain
objects, UI pages, project repositories, or audio engines directly.

Platform MIDI adapters are not part of `src/midi`. Linux raw MIDI, ALSA, or any
other device-specific polling and file descriptor code belongs under
`src/platform/...` and may only emit platform-neutral `MidiMessage` values into
the MIDI router.

### 5.6 Capture Boundary

Capture from current track must decode the source media segment. It must not
record the mixed audio output stream.

Valid capture path:

```text
current playback fact
  -> track/source URI
  -> media segment decoder extracts requested PCM segment
  -> SampleEditor applies fade/normalize
  -> WAV is saved under groove samples
  -> sound slot is updated through command/controller
```

Invalid capture path:

```text
audio output tap
  -> record whatever is currently being played
  -> save unstable mixed output
```

First shipping implementation note:

- Groove samples are stored internally as WAV files under the XDG groove samples
  directory.
- The capture boundary is `sourceUri + start + duration -> SampleBuffer`.
- The first shipping implementation must decode at least WAV, MP3, FLAC, OGG,
  and AAC current-track sources through the media decoder boundary.
- App/UI/application bridge code must not branch on `mp3`, `flac`, `ogg`, `aac`,
  or `wav`; format knowledge belongs behind the media decoder.
- If the media decoder is unavailable or a source truly cannot be decoded,
  capture must fail visibly with a decoder error and must not pretend that
  sampling succeeded.

## 6. Mode Relationship

LoFiBox Zero has these product modes:

```text
LoFiBox Zero
  Player Mode
  Remix / DSP Mode
  Pocket Groove Mode
```

Entering Pocket Groove:

- captures current playback facts needed for capture context
- pauses current player
- does not destroy queue or now-playing facts
- transfers audio ownership to Groove engine
- shows Pocket Groove Main

Exiting Pocket Groove:

- stops Groove playback
- leaves the saved/current groove project intact
- returns to the player surface
- does not automatically resume player playback

First version must not mix current song playback with Groove output. "Jam over
current track" is a later feature and requires a separate mixing specification.

## 7. Source Ownership

Required source ownership:

```text
src/groove/
  groove_project.h/.cpp
  groove_pattern.h
  groove_track.h
  groove_step.h
  groove_sound_slot.h/.cpp
  groove_song_chain.h
  groove_transport.h/.cpp
  groove_sequencer.h/.cpp
  groove_controller.h/.cpp
  groove_commands.h/.cpp
  groove_events.h
  groove_project_repository.h/.cpp

src/audio/groove/
  sample_buffer.h/.cpp
  sample_loader.h/.cpp
  sample_capture_service.h/.cpp
  sample_editor.h/.cpp
  sample_voice.h/.cpp
  groove_mixer.h/.cpp
  groove_render_engine.h/.cpp
  offline_groove_renderer.h/.cpp
  punch_fx_processor.h/.cpp
  wav_exporter.h/.cpp

src/audio/decoder/
  audio_decoder_contract.h/.cpp
  ffmpeg_segment_decoder.h/.cpp

src/midi/
  midi_clock.h/.cpp
  midi_input_router.h/.cpp
  midi_output.h/.cpp
  midi_mapping.h/.cpp

src/ui/pages/groove/
  pocket_groove_main_view.h/.cpp
  capture_overlay.h/.cpp
  sample_edit_overlay.h/.cpp
  slice_overlay.h/.cpp
  chain_overlay.h/.cpp
  fx_overlay.h/.cpp
  midi_overlay.h/.cpp
  export_overlay.h/.cpp
  project_overlay.h/.cpp

src/app/
  app_groove_bridge.h/.cpp

src/application/
  groove_command_service.h/.cpp

src/platform/host/
  linux_raw_midi_device_adapter.h/.cpp
```

Ownership rules:

- `src/groove` owns project, pattern, step, chain, transport, sequencing, and
  command semantics.
- `src/audio/groove` owns sample buffers, sample editing, rendering, mixing,
  punch FX processing, capture services, and WAV writing.
- `src/audio/decoder` owns media decoding adapters such as FFmpeg segment
  extraction; Groove capture consumes decoded PCM and does not learn codec
  details.
- `src/midi` owns MIDI message interpretation and command mapping.
- `src/application/groove_command_service.*` owns app-level orchestration of
  capture, sample rewrite, preview render playback, export, and project
  persistence.
- `src/platform/host` owns Linux raw MIDI device discovery and file descriptor
  polling/writing.
- `src/ui/pages/groove` owns rendering of projection structs only.
- `src/app/app_groove_bridge.*` owns mode transition, command dispatch,
  projection assembly, and integration with existing app/player behavior. It
  calls `GrooveCommandService` for operations that cross audio/repository
  boundaries.

## 8. Architecture Rules

Required dependency direction:

```text
ui/pages/groove
  depends on projection/view structs, core canvas, and UI theme
  does not depend on audio/groove, midi, platform, playback, repository

app_groove_bridge
  owns mode switching, command dispatch, and projection assembly
  may depend on groove domain and app input concepts
  must not call platform/device adapters directly
  must not directly include sample loader, sample editor, capture service, or
  WAV exporter internals

groove domain
  owns project/pattern/step/chain/command
  does not depend on UI, platform, audio backend, decoder backend, WebUI, CLI

audio/groove
  owns sample/mixer/render/export behavior
  does not depend on UI pages

midi
  maps MIDI input to GrooveCommand
  does not mutate project directly
  does not scan `/dev`, open ALSA/raw MIDI files, or include Linux system
  headers

application/groove_command_service
  composes groove domain, audio/groove operations, and project repository
  returns operation results for AppGrooveBridge projection/status

platform/device
  translates Linux evdev/framebuffer facts into app-facing input/output
  does not define Groove product objects

platform/host MIDI adapters
  translate device bytes to platform-neutral MIDI messages
  do not mutate GrooveProject or call UI pages
```

Forbidden implementations:

- all logic in one `PocketGroovePage.cpp`
- UI direct calls to sample loader
- UI direct calls to decoder
- UI direct calls to audio output
- UI direct calls to WAV exporter
- UI direct project JSON writes
- MIDI logic inside UI
- capture files written to install directories
- export files written to current working directory
- separate real-time and offline sound algorithms
- arbitrary `.so` groove extension loading
- arbitrary script execution in sound packs/templates/skins/mappings

## 9. UI Model

Pocket Groove has one home screen and transient overlays.

```text
Pocket Groove Main
  Capture Overlay
  Sample Edit Overlay
  Slice Overlay
  Chain Overlay
  FX Overlay
  MIDI Overlay
  Export Overlay
  Project Overlay
```

Rules:

- Pattern Main is always home.
- Overlays are tools, not pages in a deep navigation stack.
- Back closes the current overlay first.
- Back on Pattern Main exits Pocket Groove.
- Export progress is not canceled by Back unless a later explicit cancel rule is
  added.
- No overlay may rely on a mouse, touch, text cursor, waveform zoom tool, or
  desktop timeline.
- All UI must be readable at `320x170`.
- The UI can be visually alive, but visual motion is projection only and cannot
  define domain state.

## 10. Main View

Reference layout:

```text
+------------------------------+
| GROOVE  A1  092BPM  >  CHAIN |
+------------------------------+
| [01][02][03][04][05][06][07][08] |
| [09][10][11][12][13][14][15][16] |
|                              |
| KIK  #...#...#...#...        |
| SNR  ....#.......#...        |
| HAT  #.#.#.#.#.#.#.#.        |
+------------------------------+
| S04 SAMPLE  STEP05  VEL100   |
+------------------------------+
```

Required visible facts:

- mode label: `GROOVE`
- active pattern name
- BPM
- play state
- chain state
- 16 sound slot indicators
- selected sound slot
- three to four visible track rows
- selected track
- selected step
- step triggers
- parameter-lock hint on locked steps
- bottom status line with current slot, step, velocity, or FX state

The main view does not explain every feature. It answers:

- what mode am I in?
- what pattern am I editing?
- what slot/track/step is selected?
- is it playing?
- is chain mode involved?
- what rhythmic structure exists?

Allowed life/status animation:

- small tape reel motion while playing
- waveform flicker while capturing
- short FX shake while a punch FX is held
- MIDI plug/sync indicator while externally synced
- tape-write/progress motion while exporting

Animation must not require frame-perfect timing for correctness.

## 11. Overlay Specifications

### 11.1 Capture Overlay

Reference layout:

```text
+------------------------------+
| CAPTURE CURRENT TRACK        |
+------------------------------+
| Pos  01:24.32                |
| Len  1 BAR                   |
| Slot 04 EMPTY                |
| Name CHOP_04                 |
+------------------------------+
| OK REC   <-/-> Len   UP/DN Slot |
+------------------------------+
```

Fields:

- source: current track, local audio file, or built-in sample
- current source position
- capture length
- target slot
- generated sample name

Supported lengths:

- 1 beat
- 1 bar
- 2 bars
- 4 bars
- manual

Operations:

- Left/Right changes length.
- Up/Down changes target slot.
- OK sends `CaptureFromCurrentTrack` or `CaptureFromFile`.
- Back closes overlay without mutation.

Completion behavior:

- decoded sample is saved under XDG groove samples
- target slot is updated through command/controller
- overlay closes automatically
- main view shows selected slot and sample name

### 11.2 Sample Edit Overlay

Reference layout:

```text
+------------------------------+
| EDIT SLOT 04   CHOP_04       |
+------------------------------+
| |------######------|          |
| Start  00.120                |
| End    01.880                |
| Gain   086                   |
| Pitch  +00                   |
+------------------------------+
| OK Play  Fn Tool  Back Done  |
+------------------------------+
```

Tools:

- Trim Start
- Trim End
- Gain
- Pitch
- Fade In
- Fade Out
- Normalize
- Reverse
- Slice

Rules:

- no complex zooming waveform editor
- waveform is an overview only
- selected parameter is edited with device directional/value controls
- OK previews the edited sample
- Back accepts current edit state and returns to main unless an explicit cancel
  state is later specified
- each committing action emits a command

### 11.3 Slice Overlay

Reference layout:

```text
+------------------------------+
| SLICE SLOT 04       AUTO 08  |
+------------------------------+
| |01|02|03|04|05|06|07|08|    |
|      ^                       |
| Slice 03  00.420-00.610      |
| Assign: STEP 05              |
+------------------------------+
| OK Assign  + Auto  Back      |
+------------------------------+
```

Operations:

- Left/Right selects slice.
- Plus runs auto slice or increases auto-slice count.
- OK assigns selected slice to selected/current step.
- Back closes overlay.

Rules:

- max 16 slices per sound slot
- slice assignment writes `sliceIndex` to the target step
- slice playback uses slot mode `Slice`

### 11.4 Chain Overlay

Reference layout:

```text
+------------------------------+
| CHAIN                        |
+------------------------------+
| A1x04  A2x08  A3x02  A2x08   |
|                 ^            |
| Item 03  Pattern A3  Repeat 2|
+------------------------------+
| OK Edit  + Add  Del Remove   |
+------------------------------+
```

Operations:

- Left/Right selects chain item.
- OK edits selected item field.
- Plus adds a chain item.
- Delete removes selected item.
- value controls edit pattern index or repeat count.

Rules:

- horizontal chain is preferred over a long vertical list
- each item references an existing pattern
- repeat count must be at least 1
- playing chain follows item order and repeat counts exactly
- export chain uses the same order as play chain

### 11.5 FX Overlay

Reference layout:

```text
+------------------------------+
| PUNCH FX                     |
+------------------------------+
| 1 FILT  2 STUT  3 CRSH  4 STOP |
| 5 DLY   6 FRZ   7 REV   8 BRK  |
|                              |
| 1-8 FX TOGGLE  FN+KEY RECORD |
+------------------------------+
```

Operations:

- in the real-time input path, hold 1-8 triggers effect while held
- in preview-render implementations without key release, 1-8 toggles/arms the
  effect preview state instead of promising hold behavior
- release key releases effect only when the platform reports key release
- Fn+1-8 records effect lock to current step

Rules:

- effect hold is performance state, not project mutation
- Fn+effect writes `hasFxLock`, `fxType`, and `fxAmount` to the current step
- overlay can appear only while FX keys are held, or from a function menu

### 11.6 MIDI Overlay

Reference layout:

```text
+------------------------------+
| MIDI                         |
+------------------------------+
| Clock  INTERNAL              |
| In     CH 10                 |
| Out    OFF                   |
| Sync   ---                   |
+------------------------------+
| OK Edit   <-/-> Value   Back |
+------------------------------+
```

Fields:

- clock mode: internal, external, send
- input channel
- output channel
- sync status

Rules:

- default channel is 10
- note input maps slot 01 to C1, slot 02 to C#1, and so on
- CC mapping targets existing command semantics only
- no complex mapping editor in the first version

### 11.7 Export Overlay

Reference layout:

```text
+------------------------------+
| EXPORT WAV                   |
+------------------------------+
| Target  SONG CHAIN           |
| Format  48K / 16             |
| Norm    ON                   |
| Tail    2.0s                 |
+------------------------------+
| OK EXPORT      Back Cancel   |
+------------------------------+
```

Export progress:

```text
+------------------------------+
| EXPORTING                    |
+------------------------------+
| LATEBEAT_001.WAV             |
| ############.... 78%         |
|                              |
+------------------------------+
```

Fields:

- target: current pattern or Song Chain
- format: 48kHz / 16-bit WAV
- normalize on/off
- include FX on/off
- tail seconds

Rules:

- OK starts offline render
- progress must be projected
- completion prompt must show output file path or display name
- Back during export does not cancel unless an explicit cancel command exists

### 11.8 Project Overlay

Reference layout:

```text
+------------------------------+
| PROJECT                      |
+------------------------------+
| SAVE   LOAD   NEW   DELETE   |
|                              |
| Current: LATEBEAT            |
+------------------------------+
| <-/-> Select   OK Run   Back |
+------------------------------+
```

Operations:

- Save current project
- Load project
- New project
- Delete project
- Rename project, when text input is available

Rules:

- project commands go through repository boundary
- UI does not write JSON directly
- auto-save updates the current project without requiring the overlay

## 12. Input Rules

Main view controls:

```text
Left/Right       select step
Up/Down          select track
OK               toggle step
Play             play/pause Groove
Back             exit Pocket Groove
Menu             open Groove function disk
Fn+Left/Right    switch pattern
Fn+Up/Down       switch sound slot
Plus/Minus       adjust current parameter
1-16             select/audition sound slot when available
```

Fallback when no direct 1-16 keys exist:

```text
Fn+Left/Right    select sound slot
OK               audition current sound when slot focus is active
```

Function entries:

```text
Capture          Capture Overlay
Edit             Sample Edit Overlay
Slice            Slice Overlay
Chain            Chain Overlay
FX               FX Overlay
MIDI             MIDI Overlay
Export           Export Overlay
Project          Project Overlay
```

Back priority:

```text
if overlay is open:
    close overlay and return to main
else if main view:
    exit Pocket Groove
else if exporting:
    ignore Back unless explicit cancel exists
```

Mutation rule:

```text
InputEvent -> logical action -> PocketGrooveCommand -> domain/app dispatch
```

No UI control may directly edit project fields.

## 13. Data Model

### 13.1 GrooveProject

```cpp
struct GrooveProject {
    std::string id;
    std::string name;

    uint16_t bpm = 90;
    uint8_t swing = 0;

    uint8_t activePattern = 0;

    std::array<GrooveSoundSlot, 16> sounds;
    std::array<GroovePattern, 16> patterns;

    GrooveSongChain songChain;

    GrooveMidiSettings midi;
    GrooveExportSettings exportSettings;

    std::uint64_t createdAt = 0;
    std::uint64_t updatedAt = 0;
};
```

Rules:

- `id` is stable inside file names and sample URI derivation.
- `name` is user-facing and may be renamed.
- `bpm` valid range is 40 to 300 unless later changed.
- `swing` valid range is 0 to 75.
- `activePattern` valid range is 0 to 15.

### 13.2 GrooveSoundSlot

```cpp
enum class GrooveSoundType {
    Empty,
    BuiltinSample,
    UserSample,
    CapturedFromTrack,
    RecordedInput
};

enum class GroovePlaybackMode {
    OneShot,
    Gate,
    Loop,
    Slice
};

struct GrooveSoundSlot {
    GrooveSoundType type = GrooveSoundType::Empty;

    std::string id;
    std::string name;
    std::string sourceUri;

    GroovePlaybackMode mode = GroovePlaybackMode::OneShot;

    float gain = 1.0f;
    float pitchSemitone = 0.0f;
    float pan = 0.0f;

    double startSeconds = 0.0;
    double endSeconds = 0.0;

    double fadeInMs = 0.0;
    double fadeOutMs = 4.0;

    bool normalized = false;
    uint8_t chokeGroup = 0;

    std::vector<SampleSlice> slices;
};
```

Slot rules:

- there are always 16 slots
- empty slots are valid and must render clearly
- slot indexes are zero-based in code and stored JSON
- UI may display slots as 01 to 16
- source URI must be stable enough to reload project

### 13.3 SampleSlice

```cpp
struct SampleSlice {
    std::string id;
    std::string name;

    double startSeconds = 0.0;
    double endSeconds = 0.0;

    int8_t pitchSemitone = 0;
    float gain = 1.0f;
};
```

Slice rules:

- max 16 slices per slot in first version
- `endSeconds` must be greater than `startSeconds`
- slice index stored on a step references the selected slot's slice list

### 13.4 GroovePattern

```cpp
struct GroovePattern {
    std::string name = "A1";
    uint8_t length = 16;

    std::array<GrooveTrack, 16> tracks;
};
```

Pattern rules:

- first version uses 16-step patterns
- pattern length remains stored to allow future shorter/longer patterns
- pattern indexes are 0 to 15
- default display names are A1 to A16 unless later renamed

### 13.5 GrooveTrack

```cpp
struct GrooveTrack {
    uint8_t soundSlot = 0;

    std::array<GrooveStep, 16> steps;

    float gain = 1.0f;
    float pan = 0.0f;

    bool mute = false;
    bool solo = false;
};
```

Track rules:

- each track points to one sound slot by default
- track mute prevents trigger events
- track solo behavior must be defined before it affects render/export

### 13.6 GrooveStep

```cpp
struct GrooveStep {
    bool trigger = false;

    uint8_t velocity = 100;
    int8_t pitchSemitone = 0;
    int8_t microTiming = 0;

    uint8_t sliceIndex = 0;

    bool hasGainLock = false;
    float gain = 1.0f;

    bool hasPanLock = false;
    float pan = 0.0f;

    bool hasFilterLock = false;
    float filterCutoff = 1.0f;

    bool hasFxLock = false;
    uint8_t fxType = 0;
    float fxAmount = 0.0f;
};
```

Parameter-lock rules:

- a lock only applies when its `has...Lock` flag is true
- unset lock fields must not override track/slot defaults
- Fn/Write plus parameter edit writes the selected step lock
- ordinary parameter edit outside lock mode edits the selected slot or track,
  depending on current focus

### 13.7 Song Chain

```cpp
struct GrooveSongChainItem {
    uint8_t patternIndex = 0;
    uint8_t repeats = 1;

    bool muteMaskEnabled = false;
    uint16_t muteMask = 0;

    std::string label;
};

struct GrooveSongChain {
    std::vector<GrooveSongChainItem> items;
    bool enabled = false;
    uint16_t currentItem = 0;
};
```

Chain rules:

- empty chain is valid but cannot be exported as a chain without fallback
- if chain is empty, export may fall back to current pattern
- repeat count minimum is 1
- item order is the song order

### 13.8 MIDI Settings

```cpp
enum class MidiClockMode {
    Internal,
    External,
    Send
};

struct GrooveMidiSettings {
    MidiClockMode clockMode = MidiClockMode::Internal;

    uint8_t inputChannel = 10;
    uint8_t outputChannel = 10;

    bool noteInputEnabled = true;
    bool clockInputEnabled = false;
    bool clockOutputEnabled = false;

    std::array<uint8_t, 16> slotNoteMap;
};
```

Default note map:

```text
Slot 01 -> C1  / MIDI note 36
Slot 02 -> C#1 / MIDI note 37
Slot 03 -> D1  / MIDI note 38
...
Slot 16 -> D#2 / MIDI note 51
```

### 13.9 Export Settings

```cpp
enum class GrooveExportTarget {
    CurrentPattern,
    SongChain
};

struct GrooveExportSettings {
    GrooveExportTarget target = GrooveExportTarget::SongChain;

    uint32_t sampleRate = 48000;
    uint16_t bitDepth = 16;

    bool normalize = true;
    bool includeMasterFx = true;
    bool includeTail = true;

    double tailSeconds = 2.0;
};
```

First-version export format:

```text
48 kHz
16-bit PCM WAV
stereo
```

## 14. Command Model

All device input and MIDI input must become commands before project mutation.

```cpp
enum class PocketGrooveCommandType {
    EnterGroove,
    ExitGroove,

    PlayPause,
    Stop,

    SetBpm,
    SetSwing,

    SelectPattern,
    SelectTrack,
    SelectStep,
    ToggleStep,

    SetStepVelocity,
    SetStepPitch,
    SetStepGain,
    SetStepSlice,

    SelectSoundSlot,
    TriggerSoundSlot,
    AssignSoundToSlot,
    EditSoundSlot,

    CaptureFromCurrentTrack,
    CaptureFromFile,
    TrimSampleStart,
    TrimSampleEnd,
    NormalizeSample,
    ReverseSample,
    AutoSliceSample,
    AssignSliceToStep,

    TriggerPunchFx,
    ReleasePunchFx,
    RecordPunchFxToStep,

    AddSongChainItem,
    RemoveSongChainItem,
    SetSongChainPattern,
    SetSongChainRepeats,
    PlaySongChain,

    SetMidiClockMode,
    SetMidiInputChannel,
    SetMidiOutputChannel,

    ExportWav,

    SaveProject,
    LoadProject,
    NewProject,
    DeleteProject
};
```

`TriggerSoundSlot` is required even though it was not in the earliest sketch:
MIDI note input and slot audition are live triggers, not project edits.

Command payload rules:

- selection commands carry pattern, track, step, or slot indexes
- step edit commands apply to selected step unless payload explicitly specifies
  target step
- capture commands carry source facts and target slot
- export command carries target path or export request facts
- project commands carry project id/name when needed

Command results should emit structured events:

- entered/exited
- project changed
- playback changed
- selection changed
- sound triggered
- capture requested/progress/done/failed
- export requested/progress/done/failed
- error

## 15. Project JSON Format

The project file is JSON with `schema_version: 1`.

The JSON is persistence, not an external command protocol. Field order is not
semantic. Unknown fields may be ignored if schema migration rules allow it.

Representative shape:

```json
{
  "schema_version": 1,
  "id": "groove-20260511-001",
  "name": "Late Beat",
  "bpm": 92,
  "swing": 12,
  "active_pattern": 0,
  "sounds": [
    {
      "slot": 0,
      "type": "captured_from_track",
      "name": "CHOP_01",
      "source_uri": "lofibox-sample://groove-20260511-001/chop-01.wav",
      "mode": "one_shot",
      "gain": 1.0,
      "pitch": 0,
      "pan": 0,
      "start": 0.0,
      "end": 1.82,
      "fade_in_ms": 2.0,
      "fade_out_ms": 4.0,
      "slices": []
    }
  ],
  "patterns": [
    {
      "name": "A1",
      "length": 16,
      "tracks": [
        {
          "sound_slot": 0,
          "gain": 1.0,
          "pan": 0.0,
          "steps": [
            {
              "trigger": true,
              "velocity": 110,
              "pitch": 0,
              "slice": 0
            }
          ]
        }
      ]
    }
  ],
  "song_chain": {
    "enabled": true,
    "items": [
      { "pattern": 0, "repeats": 4, "label": "INTRO" },
      { "pattern": 1, "repeats": 8, "label": "BEAT" }
    ]
  },
  "midi": {
    "clock_mode": "internal",
    "input_channel": 10,
    "output_channel": 10,
    "note_input_enabled": true
  },
  "export": {
    "target": "song_chain",
    "sample_rate": 48000,
    "bit_depth": 16,
    "normalize": true,
    "include_master_fx": true,
    "include_tail": true,
    "tail_seconds": 2.0
  }
}
```

Required persistence behavior:

- save must write valid JSON
- load must repair or reject invalid indexes
- load must not execute any content
- schema migration must be explicit when schema changes

## 16. Persistence Paths

Runtime user data must follow XDG.

```text
Projects:
  ~/.local/share/lofibox/groove/projects/

Samples:
  ~/.local/share/lofibox/groove/samples/

Sound packs:
  ~/.local/share/lofibox/groove/soundpacks/

Templates:
  ~/.local/share/lofibox/groove/templates/

Cache:
  ~/.cache/lofibox/groove/

Config:
  ~/.config/lofibox/groove.json

Exports:
  ~/Music/LoFiBox/Exports/

Export fallback:
  ~/.local/share/lofibox/exports/
```

Rules:

- never write to `/usr`, `/opt`, install directories, or current working
  directory
- create directories as needed
- if `~/Music/LoFiBox/Exports/` is not writable, use fallback
- export file naming:

```text
<ProjectName>-YYYYMMDD-HHMMSS.wav
```

## 17. Capture Model

### 17.1 CaptureRequest

```cpp
struct SampleCaptureRequest {
    std::string sourceTrackId;
    std::string sourceUri;

    double startSeconds = 0.0;
    double durationSeconds = 0.0;

    uint8_t targetSoundSlot = 0;

    bool normalize = true;
    double fadeInMs = 2.0;
    double fadeOutMs = 4.0;
};
```

### 17.2 CaptureResult

```cpp
struct SampleCaptureResult {
    bool ok = false;

    std::string sampleId;
    std::string sampleUri;
    std::string displayName;

    double durationSeconds = 0.0;
    std::string errorMessage;
};
```

### 17.3 Capture Flow

```text
Current Playback Fact
  -> TrackSource / source URI
  -> Decoder extracts PCM segment
  -> SampleEditor applies fade and normalize
  -> WavExporter writes groove sample
  -> GrooveController assigns slot
  -> AppGrooveBridge rebuilds projection
  -> UI returns to main
```

Rules:

- duration must be positive
- start position must be valid for the source
- target slot must be 0 to 15
- capture does not depend on live output volume, EQ, or speaker path
- capture may use track metadata for generated display names
- failed capture returns an error event and leaves the project unchanged

## 18. Sample Editor

Required operations:

- trim start
- trim end
- gain
- pitch
- normalize
- fade in
- fade out
- reverse
- auto slice
- manual slice
- assign slice to step

Sample editor API shape:

```cpp
class SampleEditor {
public:
    SampleEditResult trim(
        const SampleBuffer& input,
        double startSeconds,
        double endSeconds);

    SampleEditResult normalize(
        const SampleBuffer& input,
        float targetPeak = 0.95f);

    SampleEditResult fadeIn(
        const SampleBuffer& input,
        double fadeMs);

    SampleEditResult fadeOut(
        const SampleBuffer& input,
        double fadeMs);

    SampleEditResult reverse(
        const SampleBuffer& input);

    std::vector<SampleSlice> autoSlice(
        const SampleBuffer& input,
        uint8_t maxSlices);
};
```

Auto-slice first-version algorithm:

```text
short-window energy
  -> energy jump detection
  -> merge cuts that are too close
  -> cap at 16 slices
```

Rules:

- operations return structured success/failure
- editor does not know UI
- editor does not write project JSON
- destructive edits must be mediated by command/controller/repository policy

## 19. Pattern And Parameter Locks

Pattern facts:

- 16 patterns per project
- 16 tracks per pattern
- 16 steps per pattern
- UI shows 3 to 4 rows at a time
- Up/Down scrolls/selects tracks

Step facts:

- trigger on/off
- velocity
- pitch
- micro timing
- slice index
- optional gain lock
- optional pan lock
- optional filter lock
- optional FX lock

Parameter-lock operation:

```text
select step
hold Fn / Write
adjust parameter
release
parameter is written to current step lock
```

Rules:

- parameter lock must not silently edit the whole slot
- ordinary parameter edit must not silently create a step lock
- locked steps must have a visible hint in the grid
- offline export must honor locks

## 20. Sequencer Timing

Base timing:

```text
seconds_per_beat = 60 / BPM
seconds_per_step = seconds_per_beat / 4
```

Swing:

- applies to off steps by delaying them
- valid range is 0 to 75
- timing must be shared by live sequencing and offline rendering

Micro timing:

- stored on the step
- offsets the trigger time within a musically bounded range
- must be included in live and offline event collection

Song Chain playback:

```text
for each chain item:
    for repeat in repeats:
        play pattern item.patternIndex
```

Export order must match play order.

## 21. Punch-In FX

Required first-version FX:

```text
1 Filter
2 Stutter
3 Bitcrush
4 Tape Stop
5 Delay Throw
6 Reverb Freeze
7 Reverse
8 Vinyl Brake
```

Operation:

```text
1-8       hold to trigger
release   restore dry/current state
Fn+1-8    record FX lock to current step
```

Rules:

- FX hold is temporary performance state
- FX record is project mutation through command
- FX must be represented separately from EQ presets
- exported WAV must include recorded FX locks when `includeMasterFx` is true
- live and offline processing share the same effect definitions

## 22. MIDI

First-version MIDI capabilities:

- MIDI Clock In
- MIDI Clock Out
- Start
- Stop
- Continue
- Note In to trigger sound slots
- basic CC to existing Groove commands

Non-goals:

- MIDI file import
- MIDI file export
- complex mapping editor
- full multichannel routing

Defaults:

```text
Channel 10
Slot 01 -> C1
Slot 02 -> C#1
Slot 03 -> D1
...
```

Mapping config path:

```text
~/.config/lofibox/groove-midi-map.json
```

Allowed mapping:

- MIDI note to `TriggerSoundSlot`
- MIDI CC to existing parameter command
- transport messages to play/stop/continue commands

Forbidden mapping:

- direct project object mutation
- direct audio engine calls
- direct UI manipulation
- script execution

## 23. Audio Render And Export

Real-time path:

```text
GrooveSequencer
  -> Trigger Events
  -> SampleVoicePool
  -> GrooveMixer
  -> PunchFxProcessor
  -> MasterLimiter
  -> AudioOutput
```

Preview render playback path:

```text
GrooveProject
  -> OfflineGrooveRenderer
  -> GrooveRenderEngine
  -> cache/preview.wav
  -> existing audio backend playFile
```

Offline export path:

```text
GrooveProject
  -> Pattern / SongChain
  -> OfflineGrooveRenderer
  -> GrooveRenderEngine
  -> WavExporter
```

Rules:

- `GrooveRenderEngine` is shared
- preview render playback is an interim playback sink, not a real-time
  groovebox audio engine
- export must render offline
- export must produce playable WAV
- export supports current pattern and Song Chain
- export defaults to 48kHz / 16-bit PCM WAV
- export includes FX when configured
- export includes tail when configured
- normalization is optional but defaults on
- limiter or clamp protection must prevent invalid PCM output

## 24. Open Extension Guide

Pocket Groove supports data-only extension points. Extension points must not
become arbitrary code execution.

### 24.1 Sound Pack

Directory:

```text
~/.local/share/lofibox/groove/soundpacks/
```

Example:

```text
~/.local/share/lofibox/groove/soundpacks/my-lofi-kit/
  soundpack.json
  kick.wav
  snare.wav
  hat.wav
  perc.wav
```

Manifest:

```json
{
  "schema_version": 1,
  "id": "community.my-lofi-kit",
  "name": "My LoFi Kit",
  "author": "vicliu",
  "license": "CC0",
  "sounds": [
    {
      "name": "KICK",
      "file": "kick.wav",
      "slot_hint": 0,
      "gain": 1.0,
      "mode": "one_shot"
    }
  ]
}
```

Rules:

- audio files and manifest only
- no code
- no network access
- no access to user music library
- no project mutation outside explicit import/install flow

### 24.2 Groove Template

Directory:

```text
~/.local/share/lofibox/groove/templates/
```

Template:

```json
{
  "schema_version": 1,
  "id": "template.lofi-boom-bap",
  "name": "LoFi Boom Bap",
  "bpm": 86,
  "swing": 18,
  "patterns": [
    {
      "name": "A1",
      "tracks": [
        { "slot": 0, "steps": "1000100010001000" },
        { "slot": 1, "steps": "0000100000001000" }
      ]
    }
  ],
  "chain": [
    { "pattern": "A1", "repeats": 8 }
  ]
}
```

Rules:

- template describes BPM, swing, pattern, and chain
- no scripts
- no filesystem access beyond loading the template file
- no network access

### 24.3 Punch FX Preset

First version ships with eight built-in FX. Future FX preset manifests may
describe parameters for existing FX types.

Example:

```json
{
  "schema_version": 1,
  "id": "fxpreset.deep-stutter",
  "name": "Deep Stutter",
  "fx": "stutter",
  "params": {
    "division": "1/32",
    "feedback": 0.35,
    "mix": 0.85
  }
}
```

Rules:

- preset data only
- no arbitrary DSP code
- no arbitrary `.so` loading
- high-performance DSP extension, if ever allowed, must go through a fixed
  plugin extension point, not raw ABI loading into the main process

### 24.4 UI Skin

Skins may change:

- colors
- icons
- fonts
- slot indicator style
- step-dot style
- animation resources

Skins may not change:

- input logic
- project data model
- audio render logic
- export logic
- MIDI behavior
- command semantics

### 24.5 MIDI Mapping

Config path:

```text
~/.config/lofibox/groove-midi-map.json
```

Example:

```json
{
  "schema_version": 1,
  "input_channel": 10,
  "slot_notes": {
    "0": 36,
    "1": 38,
    "2": 42,
    "3": 46
  },
  "cc": {
    "20": "selected_slot_gain",
    "21": "selected_slot_pitch",
    "22": "filter_cutoff",
    "23": "fx_amount"
  }
}
```

Rules:

- maps MIDI note/CC to existing Groove commands only
- cannot call internal objects directly
- cannot execute scripts
- cannot bypass command boundary

## 25. Testing Requirements

### 25.1 Domain Tests

- `GrooveProject` serialization
- default project construction
- project load repair/rejection for invalid indexes
- pattern step edit
- parameter-lock creation
- Song Chain ordering
- BPM timing
- swing timing
- micro timing

### 25.2 Audio Tests

- WAV load
- sample trim
- sample normalize
- fade in
- fade out
- reverse
- auto slice
- sample voice playback
- mixer sum
- limiter/clamp
- punch FX processing
- offline render duration
- WAV export header
- exported file loadable by ordinary WAV loader

### 25.3 Capture Tests

- capture current track position request
- capture manual range request
- invalid range handling
- decode failure handling
- captured sample assigned to target slot
- sample file saved under XDG data path
- failed capture leaves project unchanged

### 25.4 MIDI Tests

- MIDI clock tick sync
- Start
- Stop
- Continue
- note input triggers sound slot command
- wrong channel ignored
- CC maps to existing command

### 25.5 UI Tests

- `320x170` main view readable
- Capture overlay readable
- Sample Edit overlay readable
- Slice overlay readable
- Chain overlay readable
- FX overlay readable
- MIDI overlay readable
- Export overlay readable
- Project overlay readable
- Back from overlay returns to main
- overlay render code does not mutate project

### 25.6 Boundary Tests

- no WebUI groove route
- no WebSocket groove state stream
- no CLI groove command
- UI code does not include audio/groove internals
- MIDI code does not include UI pages
- app bridge pauses player on enter
- existing Player, Library, EQ, Remix, and Remote Source tests remain passing

## 26. Implementation Staging Rules

This spec defines the target feature. Implementation may be staged, but staging
must not change product meaning.

Allowed staging:

- add spec
- add domain model and tests
- add command/controller skeleton
- add sample editor and WAV tests
- add offline renderer using placeholder sample bank
- add UI projection/render tests
- wire app bridge
- later replace placeholder decoders/output with real platform integrations

Forbidden staging:

- shipping a step grid and calling it Pocket Groove complete
- adding WebUI/CLI controls because device UI is not wired yet
- putting direct sample loading in UI as a temporary shortcut
- writing samples to current working directory as a shortcut
- implementing export with a separate simplified sound engine
- silently dropping MIDI, Song Chain, punch FX, or export from the product
  definition

Any incomplete implementation must be described as scaffolding or partial
implementation in status docs or PR notes.

## 27. Acceptance Criteria

Pocket Groove is accepted only when all of the following are true:

1. Device UI can enter Pocket Groove.
2. Entering Pocket Groove pauses current player.
3. Exiting Pocket Groove returns to Player Mode without auto-resume.
4. User can capture from current song into a sound slot.
5. User can capture from a local audio file.
6. User can edit sample trim, gain, pitch, normalize, fade, reverse, and slice.
7. User can use 16 sound slots.
8. User can write 16-step patterns.
9. User can write parameter locks.
10. User can save, load, rename, delete, and auto-save projects.
11. User can arrange Song Chain.
12. User can play Song Chain.
13. User can trigger punch-in FX live.
14. User can record punch-in FX to steps.
15. MIDI clock in/out works.
16. MIDI Start, Stop, and Continue work.
17. MIDI note input triggers sound slots.
18. MIDI CC maps to existing Groove commands.
19. User can export current pattern WAV.
20. User can export Song Chain WAV.
21. Exported WAV is playable by ordinary players.
22. Exported WAV broadly matches device playback.
23. `320x170` UI is readable and operable.
24. No WebUI groove feature exists.
25. No CLI groove feature exists.
26. No WebSocket groove state feature exists.
27. Existing Player, Library, EQ, Remix, and Remote Source behavior is not
    broken.
28. New tests pass.
29. Existing relevant tests pass.

## 28. AI And Developer Constraints

When implementing Pocket Groove:

- return to this spec before adding a new object, service, adapter, or shortcut
- keep UI as projection
- keep domain independent from UI and platform
- keep audio rendering shared between live and export
- keep MIDI behind commands
- keep extension points data-only
- keep runtime paths XDG-compliant
- do not let a local workaround rewrite the product definition
- do not treat the current partial skeleton as feature completion
