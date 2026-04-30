<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Terminal UI Specification

## 1. Purpose

This document defines `LoFiBox TUI`, the terminal-native presentation target for
LoFiBox Zero.

The TUI is not a debug print view, a GUI fallback, or a second player. It is a
formal presentation target alongside the Linux framebuffer/device target and the
X11 desktop-widget target.

The intended entry points are:

```text
lofibox-tui
lofibox tui [view]
```

The TUI exists for SSH control, desktop terminals, small Linux devices,
headless servers, embedded Linux deployments, developer workflows, and future
runtime-state observation by automation and MCP-style agents.

## 2. Authority

Use:

- `project-architecture-spec.md` for source ownership.
- `runtime-command-session-architecture-spec.md` for live runtime truth.
- `lofibox-cli-surface-spec.md` for one-shot CLI behavior.
- `lofibox-zero-page-spec.md`, `lofibox-zero-layout-spec.md`, and
  `lofibox-zero-visual-design-spec.md` for existing small-screen GUI surface
  language.

This specification defines only the terminal-native presentation surface and
its adapter boundaries.

## 3. Distinction Contract

### 3.1 Current Confusions

- A terminal UI can be confused with a one-shot ASCII CLI command.
- A terminal UI can be confused with a fallback for missing X11 or framebuffer
  support.
- A terminal UI can be tempted to read playback, lyrics, remote, library, or
  audio internals directly because it runs in a terminal.
- Terminal key handling can be confused with product commands.
- Rendered text can be confused with runtime truth.

### 3.2 Valid Distinctions

- `TUI` is a presentation target.
- `CLI` is a one-shot command adapter.
- `RuntimeSnapshot` is live runtime truth before terminal rendering.
- `RuntimeCommand` is the only mutation path for live playback, queue, EQ,
  remote session, and live settings from the TUI.
- `TuiModel` may store terminal-local projection state: current view, focus,
  scroll, search/command buffers, layout cache, help/modal state, connection
  status, and the most recent runtime snapshot.
- Terminal input events are adapter events. They become `TuiInputAction` and may
  submit `RuntimeCommand`; they are not product truth.
- Terminal frames are rendered projections. They are not runtime truth and must
  not become an automation schema.

### 3.3 Invalid Distinctions

- Do not implement a second player inside the TUI.
- Do not make TUI code call `AudioBackend`, `PlaybackBackendController`,
  `LyricsProvider`, `LibraryScanner`, `AudioDecoder`, remote provider clients,
  credential stores, or metadata/tag writeback providers directly.
- Do not make the TUI own current track, queue, EQ, remote source, library,
  lyrics, or playback state facts.
- Do not execute arbitrary shell commands from the command palette.
- Do not print raw passwords, tokens, auth headers, or secret-bearing URLs.
- Do not stop playback merely because the TUI exits.

## 4. Normative Architecture

```text
Terminal
  |
  v
lofibox-tui / lofibox tui
  |
  v
TUI App / Model / Renderer / Input Router
  |
  v
RuntimeCommandClient
  |
  v
Runtime socket / Runtime command bus / Runtime query dispatcher
  |
  v
Playback / Queue / EQ / Remote Session / Settings / Runtime Projections
```

The TUI consumes runtime snapshots and submits runtime commands. It may not
construct application services, runtime domains, runtime bus/server objects, GUI
contexts, or platform host providers.

The first implementation is an ANSI terminal renderer with no default dependency
on `ncurses` or `FTXUI`. Future optional renderers may be added behind explicit
CMake options, but they must not redefine TUI semantics.

## 5. Runtime Projection Contract

The runtime snapshot must be rich enough for TUI, CLI, automation, and future
MCP-style state readers to share semantics.

The full runtime snapshot includes:

```text
playback
queue
eq
remote
settings
visualization
lyrics
library
sources
diagnostics
creator
version
```

The current implementation must at least project existing code facts:

- playback title, artist, album, source label, elapsed/duration, live/seekable,
  shuffle/repeat, audio-active, and stream metadata facts when available
- queue active ids plus visible queue items with title, artist, album, duration,
  source label, active/playable flags
- EQ enabled, preset, and 10 band values
- remote profile, source label, connection, redacted URL, buffer state, codec,
  bitrate, live, and seekable facts
- visualization availability and 10 spectrum bands from the playback session
- lyrics availability, sync status, source/provider, current line index, offset,
  visible lines, and status message from the playback session
- library track/artist/album counts and readiness/degraded facts
- source count and active source summary facts
- diagnostics capability facts, warnings, and errors
- creator analysis facts derived by the running runtime from live audio
  visualization frames: estimated BPM, spectral key estimate, LUFS estimate,
  dynamic range estimate, waveform points, beat grid, loop markers, section
  markers, stem status, analysis source, and confidence

Runtime projection code may derive these facts from application services inside
the running instance. TUI code must not derive them by reaching below runtime.

## 6. Views

The TUI must support these view identities:

```text
dashboard
now
lyrics
spectrum
queue
library
sources
eq
dsp
diagnostics
creator
help
command-palette
```

The implementation must render stable, non-crashing projections for all views.
Views whose product services are not mature yet may show honest fallback state,
but they must not fake successful service behavior. Creator view must show the
runtime creator-analysis projection when live visualization frames exist, and
must distinguish that live estimate from future offline/stem analysis.

## 7. Visual Language

LoFiBox TUI must feel like a terminal-native LoFiBox product surface, not a
modern SaaS dashboard, not a decorative cyberpunk screen, and not a rough
debug fallback.

The visual intent is:

```text
low-resolution device feel
iPod-spirit successor
Linux-first
developer friendly
retro but not crude
terminal-native
dark-background first
high information density without disorder
```

Small terminal readability has priority over ornament. Color and glyph richness
may improve scannability, but they must never become the only carrier of
state.

### 7.1 Character Sets

The TUI must support three character-set tiers:

```text
unicode  rich terminal glyphs; default when supported
ascii    box/progress/spectrum fallback for conservative terminals
minimal  very narrow/log-friendly projection
```

Unicode rich examples:

```text
┌ ─ ┐
│   │
├ ┤
▁ ▂ ▃ ▄ ▅ ▆ ▇ █
● ○ ◆ ◇ ▶ ⏸ ■
```

ASCII safe examples:

```text
+---+
|   |
+---+
[PLAYING]
###-----
```

Minimal examples:

```text
PLAYING  Track Name
01:24/03:48
```

Command/configuration sources:

```text
lofibox tui --charset unicode
lofibox tui --charset ascii
lofibox tui --charset minimal
LOFIBOX_TUI_CHARSET=unicode|ascii|minimal
```

### 7.2 Themes

The TUI must support:

```text
lofibox tui --theme dark
lofibox tui --theme light
lofibox tui --theme amber
lofibox tui --theme mono
lofibox tui --no-color
```

Theme semantics:

```text
dark   default dark terminal
amber  LoFiBox small-screen retro theme
mono   black-and-white terminal
light  light terminal compatibility
```

`--no-color` must remain fully usable. Color may emphasize, but state must also
be represented by text, glyphs, position, or labels.

## 8. Adaptive Layouts

The renderer must adapt to terminal size:

```text
wide     >= 100x30
normal   >= 80x24
compact  >= 60x16
micro    >= 40x10
tiny     >= 32x8
too-small < 32x8
```

Below `32x8`, the TUI must render a clear error with the current size and a
suggestion to use machine-readable CLI output. It must not crash:

```text
LoFiBox TUI needs at least 32x8.
Current: 24x6.
Use: lofibox now --porcelain
```

### 8.1 Wide Layout

Wide layout applies at `>=100x30`. It must render a bordered dashboard with
three side-by-side panels for Spectrum, Lyrics, and Queue:

```text
┌──────────────────────────── LoFiBox Zero ────────────────────────────┐
│ PLAYING  Night Walk - LoFi Garden                         Vol 62%    │
│ Local Library / City Lights                              EQ Focus ON │
│ 01:24 ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━────────────── 03:48             │
├──────────────────────┬──────────────────────┬───────────────────────┤
│ Spectrum             │ Lyrics               │ Queue                 │
│ ▁▂▃▅█▇▆▄▂▁           │ walking in the rain  │ > Night Walk          │
│                      │ > time moves slowly  │   Soft Signal         │
│ 31 63 125 ...        │ every sound returns  │   Amber Room          │
├──────────────────────┴──────────────────────┴───────────────────────┤
│ Source: EMBY resolved · buffer OK · 320kbps AAC · n next · q quit     │
└──────────────────────────────────────────────────────────────────────┘
```

### 8.2 Normal Layout

Normal layout applies at `>=80x24`. It must render separate Spectrum, Lyrics,
and Queue sections in that order:

```text
┌──────────────────── LoFiBox Zero ────────────────────┐
│ ▶ Night Walk - LoFi Garden                    Vol 62% │
│ 01:24 ━━━━━━━━━━━━━━━━━────── 03:48     EQ Focus ON   │
├──────────────── Spectrum ────────────────────────────┤
│ ▁▂▃▅█▇▆▄▂▁                                            │
├──────────────── Lyrics ──────────────────────────────┤
│      walking through the neon rain                    │
│  >   time moves slow when you're away                 │
│      every light becomes a memory                     │
├──────────────── Queue ───────────────────────────────┤
│ next: Soft Signal / Amber Room / Low Battery          │
└───────────────────────────────────────────────────────┘
```

### 8.3 Compact Layout

Compact layout applies at `>=60x16`. It drops boxes and keeps the five
dashboard layers as text bands:

```text
LoFiBox Zero  ▶ PLAYING
Night Walk - LoFi Garden
01:24 ━━━━━━━━━──── 03:48  Vol 62%

▁▂▃▅█▇▆▄▂▁

> time moves slow when you're away

next: Soft Signal
[q quit] [space pause] [n next]
```

### 8.4 Micro Layout

Micro layout applies at `>=40x10`:

```text
▶ Night Walk
01:24/03:48 Vol62
▁▂▃▅█▇▆▄▂▁
> time moves slow...
n next · q quit
```

### 8.5 Tiny Layout

Tiny layout applies at `>=32x8`:

```text
▶ Night Walk
01:24/03:48
▁▃█▆▂
q quit
```

## 9. Dashboard Contract

The dashboard layout is normative, not decorative. When runtime is connected
and playback is active, the implementation must preserve the requested
information hierarchy:

```text
1. header / runtime status
2. now playing / progress
3. spectrum + lyrics + queue
4. source / EQ / diagnostics
5. footer shortcuts
```

Wide layout must render a three-column spectrum, lyrics, and queue band.
Normal layout must render separate Spectrum, Lyrics, and Queue sections in that
order. Compact, micro, and tiny layouts must progressively reduce detail while
keeping playback state, progress, spectrum or its fallback, current lyric or
lyrics fallback, and quit/transport hints visible where size permits.

Documentation screenshots and demo captures are not allowed to prove the TUI
with only a disconnected, empty, or no-track state unless that state is the
subject being documented. Main TUI documentation must include at least one
active-playing dashboard capture and one active lyrics capture from a running
runtime snapshot.

### 9.1 Header

The header/status line must expose:

- `LoFiBox Zero`
- runtime connection state
- current mode/view
- clock when available
- output backend when projected
- battery/CPU placeholders when projected
- network source status

Connected example:

```text
┌──────────────────── LoFiBox Zero ────────────────────┐
│ runtime: connected · output: pipewire · mode: dashboard │
```

Disconnected example:

```text
│ runtime: disconnected · retrying · /run/user/1000/lofibox.sock │
```

### 9.2 Now Playing

Now Playing must expose:

- state
- title
- artist
- album
- source label
- codec
- bitrate
- duration
- elapsed
- volume
- repeat
- shuffle
- current queue index

Examples:

```text
│ ▶ PLAYING  Night Walk - LoFi Garden                   │
│ album: City Lights · source: Local Library · FLAC      │
│ 01:24 ━━━━━━━━━━━━━━━━━━━━━────── 03:48   Vol 62%      │
│ EMPTY                                                   │
│ No track loaded. Press / to search library.             │
│ ⏸ PAUSED  Night Walk - LoFi Garden                      │
│ ◌ BUFFERING  Remote stream resolving...                 │
│ ! PLAYBACK ERROR  decoder exited unexpectedly           │
```

### 9.3 Progress Bar

Progress bars must support:

```text
━━━━━━━━━━━━──────
████████░░░░░░░░░
############------
```

Rules:

- Unicode mode uses `━` / `─`.
- ASCII mode uses `#` / `-`.
- Current time is shown on the left and duration on the right.
- Live streams show `LIVE` rather than a normal elapsed/duration bar.

Examples:

```text
01:24 ━━━━━━━━━━━━━━━━━━━━━────── 03:48
LIVE  ━━━━━━━━━━━━━━━━━━━━━━━━━  remote radio
```

### 9.4 Spectrum

The default dashboard spectrum is a 10-band runtime projection:

```text
31   63  125 250 500 1k  2k  4k  8k  16k
▂    ▃   ▅   █   ▇   ▆   ▅   ▃   ▂   ▁
```

Tall/wide terminals may render a vertical spectrum:

```text
        █
    █   █
    █ █ █ █
  █ █ █ █ █
█ █ █ █ █ █ █
31 63 125 250 500 1k 2k 4k 8k 16k
```

Spectrum options:

```text
lofibox tui spectrum --bands 10
lofibox tui spectrum --bands 16
lofibox tui spectrum --bands 32
lofibox tui spectrum --mode bars
lofibox tui spectrum --mode waveform
lofibox tui spectrum --mode vu
lofibox tui spectrum --peak-hold on
lofibox tui spectrum --decay fast|normal|slow
```

TUI must consume `VisualizationRuntimeSnapshot`. It must not call
`PlaybackBackendController` or an audio backend directly.

### 9.5 Lyrics

Lyrics rendering rules:

- current line centered or near-centered
- current line highlighted
- previous/next context preserved
- long lines clipped or horizontally scrolled by TUI-local state
- unavailable lyrics show source/provider status

Example:

```text
┌──────────────── Lyrics ──────────────────────────────┐
│      walking through the neon rain                    │
│      I hear the city call my name                     │
│                                                        │
│  >   time moves slow when you're away                 │
│                                                        │
│      every light becomes a memory                     │
│      every sound becomes a place                      │
└───────────────────────────────────────────────────────┘
```

Unavailable example:

```text
Lyrics unavailable
source: local none · lrclib no match · embedded none
press L to search lyrics
```

Offset example:

```text
lyrics offset: +120ms
[ and ] adjust offset
```

TUI must consume `LyricsRuntimeSnapshot`. It must not query a lyrics provider.

### 9.6 Queue

Queue rows must expose active/selected items and playable metadata:

```text
┌──────── Queue ────────┐
│ > 01 Night Walk       │
│   02 Soft Signal      │
│   03 Amber Room       │
│   04 Low Battery      │
│   05 Rain Tape        │
└───────────────────────┘
```

Queue interaction maps to runtime commands:

```text
j/k       move selection
Enter     play selected
a         add selected to queue
d         remove selected
J/K       move queue item
g/G       top/bottom
/         search queue
```

TUI must consume `QueueRuntimeSnapshot.visible_items`; it must not reconstruct
queue truth from UI pages.

### 9.7 EQ

EQ must expose 10 bands, enabled state, active preset, and runtime command
bindings:

```text
┌──────────────────── EQ: Focus ON ────────────────────┐
│ 31  63 125 250 500 1k  2k  4k  8k 16k                │
│ +6              █                                      │
│ +3          █   █   █                                  │
│  0  ───█────█───█───█────█────█────█────█────█────█    │
│ -3                                      █              │
│ -6                                                     │
└───────────────────────────────────────────────────────┘
```

Controls:

```text
Left/Right select band
Up/Down    gain +/-
0          reset selected band
r          reset all
e          enable/disable EQ
P          preset picker
```

Command mappings:

```text
EqEnable
EqDisable
EqSetBand
EqReset
EqApplyPreset
```

## 10. Page System

The TUI must contain these page identities:

```text
Dashboard
NowPlaying
Lyrics
Spectrum
Queue
Library
Sources
EQ
DSP
Settings
Diagnostics
Creator
Help
CommandPalette
```

Page switching:

```text
1 Dashboard
2 Now Playing
3 Lyrics
4 Spectrum
5 Queue
6 Library
7 Sources
8 EQ
9 Diagnostics
0 Help
Tab        next page
Shift+Tab  previous page
:          command palette
?          help
q          quit current modal / quit TUI
```

## 11. Input Contract

Global normal-mode keys:

```text
q          quit TUI only
Space      PlaybackToggle
n          QueueStep(+1)
p          QueueStep(-1)
s          PlaybackStop
Left       PlaybackSeek(elapsed - 5s)
Right      PlaybackSeek(elapsed + 5s)
ShiftLeft  PlaybackSeek(elapsed - 30s)
ShiftRight PlaybackSeek(elapsed + 30s)
r          RemoteReconnect
e          EqEnable or EqDisable depending on current snapshot
l          lyrics page
v          spectrum page
Q          queue page
/          search mode
:          command palette
?          help
Tab        next page
ShiftTab   previous page
Esc        close modal or cancel input
Enter      activate focused item
```

Vim-style navigation keys may move TUI-local focus or scroll state. They must
not mutate product truth unless an explicit activation command is submitted.

The command palette is a LoFiBox command palette, not a shell. It may map known
phrases such as `play`, `pause`, `next`, `source reconnect`, or `eq preset
focus` to runtime commands. It must not run `shell`, `exec`, or `!command`.

## 12. Terminal Adapter Contract

`src/platform/tui` owns terminal adapter details:

- alternate screen enter/leave
- cursor hide/show
- raw mode enter/restore
- ANSI cursor movement, clear, style, and diff rendering
- terminal size detection
- SIGINT/SIGTERM/SIGWINCH-safe restoration where supported
- UTF-8 committed-text decoding
- bracketed paste guard that strips terminal controls and does not execute
  pasted command-palette text until the user confirms it
- escape-sequence translation for arrows, Shift+Tab, Shift+Arrow, Ctrl+D,
  Ctrl+U, Backspace, Escape, Enter, Tab, and printable text

`src/tui` owns terminal-neutral model, layout, widget rendering, page
composition, input routing, and command-palette interpretation.

`src/targets/tui_main.cpp` is a thin entry point that composes terminal adapters
with a runtime command client.

## 13. Performance Contract

- No busy loop.
- No audio decoding in the TUI.
- No network request in the render loop.
- Normal dashboard refresh targets 10 FPS or lower.
- Spectrum refresh targets 20-30 FPS from runtime events, with bounded render
  scheduling rather than full redraw for every event.
- Resize may force a full repaint; normal frames should diff-render.
- TUI incremental memory use should stay small enough for Raspberry Pi and
  Cardputer Zero class devices.

## 14. Runtime Event Stream

The runtime event stream is part of the runtime transport contract. It is not a
TUI-private polling trick. A client sends an `event_stream` envelope over the
runtime Unix socket, and the runtime server keeps that connection open while
continuing to accept command and query clients on other connections.

The event stream publishes stable event types:

```text
runtime.connected
runtime.disconnected
runtime.snapshot
playback.changed
playback.progress
playback.error
queue.changed
eq.changed
remote.changed
settings.changed
lyrics.changed
lyrics.line_changed
visualization.frame
library.scan_started
library.scan_progress
library.scan_completed
diagnostics.changed
creator.changed
```

Each event carries:

```text
event_type
event_version
timestamp_ms
type-specific payload fields
full RuntimeSnapshot projection
```

The first event pair after connection is `runtime.connected` followed by
`runtime.snapshot`. Subsequent events are generated by comparing runtime
snapshots and by emitting heartbeats as bounded `runtime.snapshot` events.
The event version is stream-monotonic even when progress/visualization updates
do not mutate command-result version truth.

Events are not permission to redraw every byte immediately. The TUI must ingest
events into its model and schedule bounded rendering. The current TUI uses a
dedicated event ingest thread, drains events into `TuiModel`, and renders at the
configured refresh cadence.

## 15. Testing Contract

Smoke coverage must include:

- progress bar does not divide by zero
- live stream uses live progress semantics
- spectrum unavailable fallback
- lyrics unavailable fallback
- `32x8`, `80x24`, and `120x40` layouts
- Unicode and ASCII charset rendering
- runtime disconnected rendering
- global input routing for `Space`, `n`, and `q`
- command-palette paste guard: pasted text fills input but does not execute
  until `Enter`
- UTF-8 committed text decoding
- runtime event stream connection, initial snapshot, command concurrency, and
  playback/visualization event delivery
- creator analysis projection when runtime visualization frames exist
- EQ 10-band rendering
- terminal capability detection
- resize/full repaint behavior in the terminal diff renderer
