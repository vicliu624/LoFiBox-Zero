<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Current UI Implementation Specification

## 1. Purpose

This document constrains the current UI implementation for `LoFiBox Zero`.

It exists because final product meaning must be kept separate from what is implemented right now.
This file is a current implementation contract, not the final product definition.

Use this document when implementing the current screens, navigation, UI state, and near-term UI behavior.

Use `lofibox-zero-final-product-spec.md` when deciding what the product ultimately is.
Use `lofibox-zero-app-state-spec.md` when playback truth, navigation truth, library context, queue truth, search truth, source truth, or shared settings truth are involved.
Use `legacy-lofibox-product-guidance.md` when deciding whether a feature belongs in the product at all.
Use `lofibox-zero-media-pipeline-spec.md` when decode, playback-pipeline, or output behavior is involved.
Use `lofibox-zero-audio-dsp-spec.md` when EQ or DSP behavior is involved.
Use `lofibox-zero-streaming-spec.md` when remote-source or server-backed behavior is involved.
Use `project-architecture-spec.md` when deciding where code belongs architecturally.
Use `lofibox-zero-layout-spec.md` for page geometry and template ownership.
Use `lofibox-zero-visual-design-spec.md` for styling, color, and typography.

## 2. Scope

This document covers:

- page purpose
- required UI elements
- page-level state
- entry and exit rules
- navigation outcomes
- empty-state behavior
- explicit non-goals

This document does not define:

- the product's final form
- final pixel-perfect visual polish
- internal code structure
- final device packaging or deployment
- the detailed page contracts for streaming, online, or cloud features

## 3. Specification Strength

The following words are normative in this document:

- `MUST`
  Required for the page to be considered aligned with the current implementation contract.
- `SHOULD`
  Strong default unless later product decisions change it.
- `MAY`
  Optional and non-blocking.
- `MUST NOT`
  Explicitly disallowed for the current implementation contract.

## 4. Global Page Contract

### 4.1 Shared Navigation Rules

- Every actionable page `MUST` be operable with keyboard-only input.
- `Back` `MUST` return to the previous page in the navigation stack.
- `Confirm` `MUST` activate the current selection or the page's primary action.
- Selection movement `MUST` be predictable and page-consistent.
- Non-main-menu pages `MUST` expose a clear back affordance.

### 4.2 Selection And Focus Rules

- A newly entered list-like page `MUST` select the first actionable row by default unless a later specification explicitly overrides it.
- Implementations `MUST NOT` invent ad-hoc per-page remembered cursors unless that behavior is deliberately specified later.
- Non-actionable empty-state rows `MUST NOT` pretend to be interactive.

### 4.3 Truthful Status Rule

- Top-bar status indicators `MUST` represent real data when shown.
- UI `MUST NOT` show fake Wi-Fi, fake signal, fake Bluetooth, or made-up system state merely because a mockup had decorative icons.
- Battery status `SHOULD` be shown when the system can report it truthfully.

### 4.4 Density And Layout Rule

- All pages `MUST` remain readable on the shared `320x170` logical screen.
- A page `MUST NOT` inherit large-screen spacing assumptions from old `480x222` design exports.
- If a larger-screen legacy layout does not fit `320x170`, the product meaning `MUST` be preserved even when the exact composition changes.

### 4.5 Empty-State Rule

- Every data-driven page `MUST` define a stable empty state.
- Empty states `MUST` keep navigation working.
- Empty states `MUST NOT` crash, loop, or produce visually broken rows.

### 4.6 Small-Screen Text Rule

- Primary labels `MUST` be prioritized over secondary metadata when space is limited.
- Long text `SHOULD` be clipped, truncated, or selectively scrolled rather than overflowing unpredictably.
- Metadata `MUST` remain Unicode-safe and CJK-capable.

### 4.7 Page-Local F1 Help Rule

- Every page except `Boot` `MUST` accept `F1` as `EVT_HELP`.
- `F1:HELP` is a defensive programming contract, not a shared catch-all help page.
- The App shell may provide the common modal frame and close behavior, but the shortcut rows `MUST` be selected from the current page's own page specification.
- A page without explicitly defined shortcuts `MUST` still implement `EVT_HELP` and show an empty help modal such as `NO SHORTCUTS`.
- A page `MUST NOT` borrow another page's shortcut content merely because the page has no current shortcuts.
- When a help modal is open, `BACKSPACE`, `ENTER`, or `F1` `MUST` close the modal before normal page navigation or page actions run.

## 5. Application-Level Navigation Map

The current implementation page graph is:

- `Boot` -> `Main Menu`
- `Main Menu` -> `Songs(All)` via `Music`
- `Main Menu` -> `Music Index` via `Library`
- `Main Menu` -> `Playlists`
- `Main Menu` -> `Now Playing`
- `Main Menu` -> `Equalizer`
- `Main Menu` -> `Settings`
- `Music Index` -> `Artists`
- `Music Index` -> `Albums(All)`
- `Music Index` -> `Songs(All)`
- `Music Index` -> `Genres`
- `Music Index` -> `Composers`
- `Music Index` -> `Compilations`
- `Artists` -> `Albums(Filtered By Artist)`
- `Albums` -> `Songs(Album Context)`
- `Genres` -> `Songs(Genre Context)`
- `Composers` -> `Songs(Composer Context)`
- `Compilations` -> `Songs(Compilation Context)`
- `Playlists` -> `Playlist Detail`
- `Playlist Detail` -> `Now Playing`
- `Settings` -> `About`

## 6. Per-Page Specification System

### 6.1 Standard Page File Template

Each page file under `docs/specification/current-ui-pages/` should follow this contract shape:

1. `Purpose`
2. `Related Specs`
3. `Entry Conditions`
4. `Data Dependencies`
5. `State Enumeration`
6. `Required Elements`
7. `Event Contract`
8. `Transition Contract`
9. `Empty State`
10. `Error State`
11. `Non-Goals`

If a section is intentionally minimal for a specific page, the page file should still say so explicitly rather than omitting the topic by silence.

### 6.2 State And Event Contract Conventions

Page files under `docs/specification/current-ui-pages/` should use these naming rules:

- page states use `STATE_<PAGE>_<MODE>`
- events use `EVT_<ACTION>`
- guards are written inline as `[guard: ...]`
- effects are written inline as `effect: ...`

Transition bullets should use this shape:

- `STATE_X + EVT_Y [guard: ...] -> effect: ... -> STATE_Z`
- `STATE_X + EVT_Y [guard: ...] -> effect: ... -> <Target Page>`

If an event changes selection or a local value but does not change the page mode, the transition should explicitly say that it stays in the same state.

### 6.3 Canonical Shared Events

Page files should prefer this shared event vocabulary whenever it fits:

- `EVT_PAGE_ENTER`
- `EVT_PAGE_EXIT`
- `EVT_NAV_UP`
- `EVT_NAV_DOWN`
- `EVT_NAV_LEFT`
- `EVT_NAV_RIGHT`
- `EVT_CONFIRM`
- `EVT_BACK`
- `EVT_HELP`

Pages may extend this vocabulary with domain-specific events when needed, for example:

- `EVT_SYSTEM_READY`
- `EVT_PROGRESS_UPDATED`
- `EVT_PLAYBACK_TRACK_CHANGED`
- `EVT_SHUFFLE_TOGGLED`
- `EVT_REPEAT_TOGGLED`

Custom events should remain action-oriented and should not encode backend implementation details into the event name.

### 6.2 Page Specification Index

The shared rules in Sections `4`, `5`, `7`, `8`, and `9` apply to every page spec below.

Each current implementation page has its own dedicated specification file:

- `Boot`: `docs/specification/current-ui-pages/boot.md`
- `Main Menu`: `docs/specification/current-ui-pages/main-menu.md`
- `Music Index`: `docs/specification/current-ui-pages/music-index.md`
- `Artists`: `docs/specification/current-ui-pages/artists.md`
- `Albums`: `docs/specification/current-ui-pages/albums.md`
- `Songs`: `docs/specification/current-ui-pages/songs.md`
- `Genres`: `docs/specification/current-ui-pages/genres.md`
- `Composers`: `docs/specification/current-ui-pages/composers.md`
- `Compilations`: `docs/specification/current-ui-pages/compilations.md`
- `Playlists`: `docs/specification/current-ui-pages/playlists.md`
- `Playlist Detail`: `docs/specification/current-ui-pages/playlist-detail.md`
- `Now Playing`: `docs/specification/current-ui-pages/now-playing.md`
- `Equalizer`: `docs/specification/current-ui-pages/equalizer.md`
- `Settings`: `docs/specification/current-ui-pages/settings.md`
- `About`: `docs/specification/current-ui-pages/about.md`

## 7. Shared App-State Index

Cross-page shared state is defined outside page files and must not be redefined page by page.

The current shared application-state contracts are:

- `NavigationStack`: `docs/specification/app-states/navigation-stack.md`
- `PlaybackSession`: `docs/specification/app-states/playback-session.md`
- `QueueState`: `docs/specification/app-states/queue-state.md`
- `SearchState`: `docs/specification/app-states/search-state.md`
- `SourceState`: `docs/specification/app-states/source-state.md`
- `LibraryContext`: `docs/specification/app-states/library-context.md`
- `SettingsState`: `docs/specification/app-states/settings-state.md`
- `NetworkState`: `docs/specification/app-states/network-state.md`
- `MetadataServiceState`: `docs/specification/app-states/metadata-service-state.md`

Page files may project these states and trigger their events, but page files must not become the primary owner of those truths.

## 8. Deferred Features That Must Not Leak Into Current UI Pages

The following are intentionally deferred from the current UI pages and `MUST NOT` be smuggled into page implementations without an explicit product decision and page-spec update:

- search
- custom playlist creation
- queue editor
- lyrics
- streaming or server-management UI that has not yet been page-specified
- online artwork lookup
- account sync
- advanced audio-device settings
- generic plugin/settings systems

## 9. Implementation Checklists

## 9.1 A Page Is Not Done Unless

- its purpose matches this document
- its visible elements match this document
- its actions and navigation outcomes match this document
- it defines truthful empty states
- it remains operable with keyboard-only input
- it does not introduce unapproved scope

## 9.2 A Page Is Wrong If

- it copies old widget structure without preserving the correct product meaning
- it promotes a design-only idea into the current UI without a product decision
- it invents fake system state
- it adds convenience actions that were never specified
- it blurs the distinction between library browsing, playback, settings, and device adapter behavior

## 20. Final Surface Baseline In Current Implementation

As of 2026-04-27, the current small-screen page model includes these product surfaces:

- Main Menu
- Library/List browsing
- Now Playing
- Lyrics
- EQ
- Settings
- Source Manager
- Search
- Queue / Up Next
- Remote Browse
- Server Diagnostics
- Stream Detail
- Playlist Editor

Pages that are not yet pixel-specialized must still have a page identity, title, list classification where applicable, and page-specific `F1:HELP` ownership. They may reuse the shared compact list renderer, but they must not be represented as anonymous rows inside another page.
