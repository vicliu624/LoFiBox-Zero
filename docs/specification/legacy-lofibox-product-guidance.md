<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Legacy LoFiBox Product Guidance

## 1. Purpose

This document extracts the reusable product intent from the legacy `LoFiBox` repository at `C:\Users\VicLi\Documents\Projects\LoFiBox`.

Its purpose is not to revive the old codebase shape.
Its purpose is to turn the old repository's implementation, design mockups, and interaction patterns into stable product guidance for future `LoFiBox Zero` development on Linux and `M5Stack CardputerZero`.

When there is tension between this document and the old repository structure, this document wins.

For current UI implementation constraints, use `lofibox-zero-page-spec.md` alongside this document.

## 2. Source Corpus

The guidance below is derived from these source types inside the legacy repository:

- Product framing and technical intent in `README.md`
- Domain logic in `src/app/library.*`, `src/app/player.*`, and `src/app/eq_dsp.*`
- Page, navigation, and input logic in `src/ui/lofibox/*` and `src/ui/screens/*`
- Cardputer-specific interaction clues in `src/board/CardputerBoard.cpp`
- Visual intent in `images/*` and `images/design/*`

This document intentionally treats those sources with different authority levels:

- Repeated across README, code, and design: strong product signal
- Present in code but not strongly represented in design: likely product behavior, but possibly unfinished UX
- Present in design only: useful visual aspiration, but not automatically normative
- Present only because of Arduino, PlatformIO, LVGL, or board SDK structure: legacy accident, not product truth

## 3. Distinction Contract

### 3.1 Current Confusions

- The old repository mixes product truth with MCU-era technical constraints.
- The old repository contains both implemented behavior and aspirational design exports.
- The old repository sometimes uses screen names that blur user intent, especially `Music` versus `Library`.
- The old repository contains at least a few unfinished or inconsistent details, so not every line of old code is normative.

### 3.2 Valid Carry-Forward Distinctions

- `LoFiBox` is an offline-first handheld music player; Linux-capable builds may extend it with remote media access, but that extension must not turn the product into a general cloud media platform.
- `Library browsing`, `playback control`, `equalizer tuning`, and `device/settings info` are separate product responsibilities.
- `Physical shell`, `screen content`, and `device runtime` are separate concerns.
- `Track metadata`, `cover art`, `play history`, and `playback mode` are part of product behavior, not display-only decoration.
- `Smart playlists` and `user playlists` are different concepts and should not be conflated.
- `Design direction` and `current implementation status` are different signals and should be recorded separately.

### 3.3 Invalid Carry-Forward Distinctions

- Do not treat `PlatformIO`, `Arduino`, `board classes`, or old LVGL widget trees as the natural structure of `LoFiBox Zero`.
- Do not treat old display resolutions such as `240x135` or design export sizes such as `480x222` as the new truth layer.
- Do not treat fake top-bar symbols such as always-on Wi-Fi as required product behavior.
- Do not treat old bugs or incomplete pages as product decisions.
- Do not treat `Cardputer` keyboard hacks for LVGL as the final input model for Linux.

## 4. Product North Star

`LoFiBox Zero` should remain a focused, offline-first handheld music player with:

- local media playback from on-device storage
- optional Linux-side access to remote audio streams, LAN media roots, and home media servers through the same player mental model
- browse-first library navigation
- low-distraction, button-first interaction
- strong metadata and cover-art support
- a first-class equalizer experience
- small-screen readability
- predictable behavior over flashy complexity

It should not drift into:

- streaming-first behavior
- cloud account requirements
- touch-first UX assumptions
- large-system settings sprawl
- ornamental motion or UI chrome that weakens playback usability

## 5. Evidence Grades Used In This Document

- `Confirmed`
  Supported by multiple source types and safe to treat as stable product signal.
- `Adopt With Adaptation`
  Product intent is real, but exact old implementation details should not be copied literally.
- `Provisional`
  Useful design direction or partial implementation, but not strong enough to force first-release behavior.
- `Rejected`
  Legacy artifact or misleading implementation detail that should not guide new work.

## 6. Normative Product Definition

### 6.1 Core Product Promise

- `Confirmed`: The product is a lightweight handheld player for locally stored music.
- `Confirmed`: Playback should work without network dependency.
- `Adopt With Adaptation`: Linux deployments may add streaming and remote-library access, as long as local playback remains a first-class no-network path and the remote model does not fork the player experience.
- `Confirmed`: Equalizer tuning is a marquee feature, not an afterthought hidden deep in settings.
- `Confirmed`: Navigation should remain understandable and low-friction on a small screen and hardware keyboard.
- `Adopt With Adaptation`: The classic iPod-like browsing hierarchy is a strong reference, but it should be reinterpreted for `320x170` and Linux rather than copied widget-for-widget.

### 6.2 Primary User Activities

- browse the local library by category
- play a track quickly
- see current playback state at a glance
- skip, pause, shuffle, and repeat without friction
- inspect or enjoy cover art when present
- adjust equalizer bands while music is playing
- rely on simple smart playlists derived from listening behavior

## 7. Product Objects

### 7.1 Track

- `Confirmed`: A track is the main playable media item.
- `Confirmed`: Useful fields include path, title, artist, album, genre, composer, duration, added time, play count, last played, and embedded cover-art metadata.
- `Confirmed`: If tags are incomplete, track identity must still survive through filename and folder fallback heuristics.

### 7.2 Album

- `Confirmed`: Albums are browseable objects distinct from tracks.
- `Confirmed`: Album identity in the old repository is effectively `(album name, album artist)` for ordinary browsing.
- `Adopt With Adaptation`: Compilation handling may need a distinct grouping model when the same album name appears under multiple artists.

### 7.3 Artist, Genre, Composer

- `Confirmed`: These are first-class library dimensions.
- `Confirmed`: They are browse filters that lead to track or album subsets.

### 7.4 Playlist

- `Confirmed`: `On-The-Go` is a dynamic, user-facing playlist concept.
- `Confirmed`: `Recently Added`, `Most Played`, and `Recently Played` are smart playlists derived from library metadata.
- `Provisional`: Fully user-authored named playlists such as `90's Music`, `Gym Mix`, and `Favorites` appear in design exports, but they were not truly implemented in the old codebase.
- `Provisional`: `Add Playlist...` appears in design exports and should be treated as future product direction rather than current implementation requirement.

### 7.5 Player State

- `Confirmed`: Playback state includes current track, playing versus paused, playback mode, and runtime time progress.
- `Confirmed`: Playback modes are `Sequential`, `Shuffle`, and `Repeat One`.
- `Adopt With Adaptation`: A stored `volume` field existed in the old player state, but volume UX was not meaningfully designed yet. Keep room for it, but do not invent a premature volume screen without a clear product decision.

### 7.6 Equalizer

- `Confirmed`: The legacy equalizer starting point is a 6-band model.
- `Confirmed`: The effective legacy band frequencies are `60 Hz`, `200 Hz`, `400 Hz`, `1 kHz`, `2.5 kHz`, and `8 kHz`.
- `Confirmed`: Each legacy starting-point band is adjustable in the range `-12 dB` to `+12 dB`.
- `Adopt With Adaptation`: The visible labels may remain `Bass`, `200 Hz`, `400 Hz`, `1 kHz`, `2.5 kHz`, `Treble`, but the semantic mapping should stay tied to the actual six frequencies above.
- `Adopt With Adaptation`: Linux-capable builds may expand equalizer behavior into the broader DSP capability defined in `docs/specification/lofibox-zero-audio-dsp-spec.md`, as long as the dedicated EQ-page identity and live-adjustment feel survive.
- `Adopt With Adaptation`: A more complete EQ capability should include `preamp`, bypass or comparison behavior, and preset workflows rather than only band gain controls.
- `Confirmed`: The current `LoFiBox Zero` implementation target for graphic EQ is `10-band`.
- `Adopt With Adaptation`: The implementation band set should use `31 Hz`, `62 Hz`, `125 Hz`, `250 Hz`, `500 Hz`, `1 kHz`, `2 kHz`, `4 kHz`, `8 kHz`, and `16 kHz`.
- `Provisional`: `15-band` graphic EQ, `balance`, `loudness`, `ReplayGain`, `limiter`, parametric EQ, filters, device binding, and content-type binding are valid future directions beyond the current implementation contract.
- `Rejected`: The legacy `eq_input.cpp` band map inconsistency is an implementation bug, not a product rule.

## 8. Library And Metadata Specification

### 8.1 Media Source

- `Confirmed`: The player is built around local filesystem media.
- `Adopt With Adaptation`: Local filesystem media remains the default source model, but Linux-capable builds may also expose remote sources defined in `docs/specification/lofibox-zero-streaming-spec.md`.
- `Adopt With Adaptation`: The old project scanned `/music`; for `LoFiBox Zero`, `/music` should remain the default media root, but the root must be configurable for Linux deployments.

### 8.2 File Formats

- `Confirmed`: The old guaranteed format set was `MP3` and `WAV`.
- `Adopt With Adaptation`: These remain part of the guaranteed starting set, but Linux-capable builds should broaden practical playback support through the shared media pipeline defined in `docs/specification/lofibox-zero-media-pipeline-spec.md`.
- `Adopt With Adaptation`: The Linux target set should expand to include `AAC`, `WAV/PCM`, `FLAC`, `Ogg Vorbis`, `Opus`, `ALAC`, `APE`, and `AIFF` without changing the product's basic player mental model.
- `Confirmed`: Broader format support must not be implemented as page-level behavior or as scattered extension-based logic.

### 8.3 Scan Behavior

- `Confirmed`: Library building is recursive and directory-based.
- `Confirmed`: The scan process should collect metadata before normal browsing begins.
- `Adopt With Adaptation`: The old boot flow blocked on scan and enforced a minimum splash duration. On Linux, keep the concept of a branded loading state, but do not preserve an arbitrary delay if the system can become interactive sooner.

### 8.4 Metadata Extraction

- `Confirmed`: Title, artist, album, genre, and composer should be extracted from embedded metadata when possible.
- `Confirmed`: Embedded cover-art metadata is important and should be preserved as a first-class capability.
- `Confirmed`: Metadata decoding must handle Unicode safely enough for non-ASCII libraries.
- `Adopt With Adaptation`: Chinese and broader CJK support is part of the intent signal, because the old project shipped a `Noto Sans SC` font path and UTF-aware tag decoding.
- `Adopt With Adaptation`: Metadata extraction should converge through a shared pipeline-level `MetadataProvider` rather than being reimplemented separately for each format or source family.

### 8.5 Fallback Heuristics

- `Confirmed`: Missing title falls back to filename without extension.
- `Confirmed`: Missing album may fall back to the immediate parent directory.
- `Confirmed`: Missing artist may fall back to the parent-of-parent directory.
- `Confirmed`: Missing genre or composer should fall back to explicit `Unknown` values rather than blank UI.

### 8.6 Listening History

- `Confirmed`: `added_time`, `play_count`, and `last_played` are part of the product model.
- `Confirmed`: These fields power smart playlists and should not be demoted to debug-only metadata.

## 9. Screen And Flow Specification

### 9.1 Global Screen Map

| Screen | Status | Purpose | Primary Actions |
| --- | --- | --- | --- |
| Boot | Confirmed | Establish branded loading state while media becomes available | Show boot identity, communicate loading |
| Main Menu | Confirmed | First entry point into the product | Move across featured destinations, confirm selection |
| Music Index | Confirmed | Browse library categories | Open artists, albums, songs, genres, composers, compilations |
| Artists | Confirmed | Browse artists alphabetically | Open artist albums |
| Albums | Confirmed | Browse all albums or albums by artist | Open album tracks |
| Songs | Confirmed | Browse playable tracks in current context | Start playback |
| Genres | Confirmed | Browse genres | Open tracks in genre |
| Composers | Confirmed | Browse composers | Open tracks by composer |
| Compilations | Adopt With Adaptation | Browse multi-artist albums | Open compilation tracks |
| Playlists | Confirmed | Browse smart and possibly user playlists | Open playlist contents |
| Playlist Detail | Confirmed | List tracks in selected playlist | Start playback from playlist |
| Now Playing | Confirmed | Show current playback focus | Pause, skip, shuffle, repeat, inspect progress |
| Equalizer | Confirmed | Tune six-band EQ | Select band, adjust band |
| Settings | Confirmed | Surface a small set of device and player preferences | Adjust options, open about |
| About | Confirmed | Show product/storage/version/device information | Read-only inspection |

### 9.2 Boot Flow

- `Confirmed`: The product should have a branded startup experience.
- `Confirmed`: Startup should communicate that the library is loading.
- `Adopt With Adaptation`: The old `logo + Loading...` splash is a useful starting point, but Linux startup may need asynchronous progression or partial availability rather than a single blocking screen.

### 9.3 Main Menu

- `Confirmed`: The front page is a carousel-like featured menu rather than a plain list.
- `Confirmed`: The old top-level entries were `Music`, `Library`, `Playlists`, `Now Playing`, `Equalizer`, and `Settings`.
- `Confirmed`: The page is icon-driven and center-focused.
- `Confirmed`: The center item is primary, side items are visible previews, and pagination dots reinforce menu position.

#### Important Interpretation

- `Adopt With Adaptation`: In the old code, `Music` meant quick access to all songs, while `Library` meant category browsing.
- `Adopt With Adaptation`: The behavioral distinction is useful and should survive, but the naming may be revisited later if it confuses users.

### 9.4 Music Index

- `Confirmed`: The library browse hub includes `Artists`, `Albums`, `Songs`, `Genres`, `Composers`, and `Compilations`.
- `Provisional`: Design exports also showed `Playlists` in this area, even though old code kept `Playlists` separate.
- `Adopt With Adaptation`: `Playlists` should remain a first-class destination; whether it lives only on the main menu or also inside the music index can be decided later without changing the domain model.

### 9.5 Artist, Album, Song, Genre, Composer Flows

- `Confirmed`: `Artists -> Albums -> Songs` is a real browse path.
- `Confirmed`: `Albums -> Songs` is a real browse path.
- `Confirmed`: `Genres -> Songs` and `Composers -> Songs` are direct browse paths.
- `Confirmed`: All browse lists are selection-based and keyboard navigable.

### 9.6 Compilations

- `Adopt With Adaptation`: The old logic inferred compilations by detecting the same album name under multiple artists.
- `Confirmed`: A compilations concept is valid and should remain available.
- `Adopt With Adaptation`: The new implementation should model it intentionally rather than copying the old brittle page logic.

### 9.7 Playlists

- `Confirmed`: The product should expose `On-The-Go`, `Recently Added`, `Most Played`, and `Recently Played`.
- `Confirmed`: Selecting a playlist should open a track list, not a secondary menu.
- `Confirmed`: `On-The-Go` was auto-populated when a track was played, and that behavior is a useful carry-forward feature.
- `Provisional`: Editable, custom, named playlists are desired direction but not required in the current implementation.

### 9.8 Now Playing

- `Confirmed`: `Now Playing` is a primary destination, not a hidden overlay.
- `Confirmed`: The screen contains cover art, title, artist, album, elapsed time, total duration, progress bar, and transport controls.
- `Confirmed`: Controls include previous, play/pause, next, shuffle, and repeat.
- `Confirmed`: The screen updates over time while playback runs.
- `Confirmed`: Missing cover art must degrade gracefully to a clean empty cover area.
- `Adopt With Adaptation`: The layout pattern of `cover on left, metadata and controls on right` is strong visual intent, but it must be re-composed for `320x170`.

### 9.9 Equalizer

- `Confirmed`: Equalizer deserves a dedicated full screen, not a tiny settings row.
- `Confirmed`: The screen should visually emphasize the six sliders and the `0 dB` line.
- `Confirmed`: A preset row belongs at the bottom of the screen conceptually, even if preset editing is deferred.
- `Adopt With Adaptation`: EQ changes should feel realtime during playback rather than like an offline configuration form.
- `Adopt With Adaptation`: If the product later grows simple, advanced, and professional DSP modes, the default entry should still feel approachable on a small device.
- `Provisional`: Preset names such as `Flat` and `Dance` appeared visually, but preset management was not truly implemented.

### 9.10 Settings

- `Confirmed`: Settings should stay small and product-relevant.
- `Confirmed`: Backlight, sleep timer, language, and about belong here.
- `Adopt With Adaptation`: Shuffle belongs to playback behavior and may appear in settings, but its more important home is `Now Playing`.
- `Provisional`: Repeat-one appeared in the old code settings list, but not in the design exports. Treat repeat as required playback behavior, but not necessarily as a permanent settings row.

### 9.11 About

- `Confirmed`: About must show storage and version information.
- `Provisional`: Design exports also showed model and richer branding treatment.
- `Adopt With Adaptation`: `About` should become a small information card or summary page rather than a raw two-row list, but the minimum truth payload remains storage, version, and device/product identity.

## 10. Interaction Specification

### 10.1 Primary Interaction Mode

- `Confirmed`: Device interaction is button-first and keyboard-friendly.
- `Confirmed`: Every essential screen must be fully operable without pointer input.
- `Rejected`: Touch-first, drag-first, or pointer-dependent interaction patterns.

### 10.2 Shared Navigation Semantics

- `Confirmed`: Selection movement is consistent across pages.
- `Confirmed`: There is a universal `back` action.
- `Confirmed`: `confirm/enter` activates the selected item or the main playback action.
- `Confirmed`: Selection highlight and focus state should be visually obvious.

### 10.3 Page-Specific Interaction Rules

- `Confirmed`: Main menu uses horizontal movement semantics.
- `Confirmed`: List pages use vertical movement semantics with scrolling when selection exceeds the visible window.
- `Confirmed`: Now Playing maps left and right to previous and next track, and confirm to play/pause.
- `Confirmed`: Equalizer uses left and right to select a band and up and down to adjust gain.

### 10.4 Legacy Cardputer Mapping Clues

The old Cardputer implementation mapped keyboard input approximately like this:

- `W` or `K`: up
- `S` or `J`: down
- `A` or `H`: back
- `D` or `L`: confirm
- `Q`: escape/back
- `,` and `.`: left and right
- `Enter`: confirm
- `Backspace/Delete`: back/delete

Decision:

- `Adopt With Adaptation`: These mappings are useful historical desktop-development hints.
- `Rejected`: These mappings are not the product truth themselves. The truth is the logical action model, not the old LVGL keycodes.

## 11. Visual And Information Design Guidance

### 11.1 Global Visual Language

- `Confirmed`: The product uses a dark, glossy, high-contrast media-device aesthetic.
- `Confirmed`: Selection emphasis is vivid and cool-toned, especially blue highlight bars and blue progress cues.
- `Confirmed`: Typography is large, legible, and simple.
- `Confirmed`: Status and chrome stay compact so content remains dominant.

### 11.2 Top Bar

- `Confirmed`: A top bar exists on most content screens.
- `Confirmed`: It carries `back context`, `page title`, and a `status area`.
- `Adopt With Adaptation`: Battery belongs in the status area when available.
- `Rejected`: Always-fake Wi-Fi indicators or invented status icons when the system cannot report them truthfully.

### 11.3 Main Menu Visuals

- `Confirmed`: The front page should feel more featured and branded than other screens.
- `Confirmed`: Large square app-style icons are part of the visual identity.
- `Confirmed`: Center focus and pagination dots are important cues.

### 11.4 List Page Visuals

- `Confirmed`: List screens should prioritize density, contrast, and immediate readability.
- `Confirmed`: A blue highlight bar with white text is the dominant selected-row treatment.
- `Confirmed`: Left-side icons are helpful but not mandatory for every row.
- `Adopt With Adaptation`: The `>` affordance on the right side is useful for rows that navigate deeper.

### 11.5 Now Playing Visuals

- `Confirmed`: Cover art is a large visual anchor when available.
- `Confirmed`: Track title should receive the strongest text emphasis.
- `Confirmed`: Progress is emphasized with a bright bar and readable time labels.
- `Confirmed`: Playback controls should be immediately discoverable without a second click.

### 11.6 Equalizer Visuals

- `Confirmed`: EQ is not a generic settings list; it is a visually expressive audio-control page.
- `Confirmed`: The `0 dB` center line should stand out.
- `Confirmed`: Warm slider colors against a dark field are a core part of the EQ identity.

### 11.7 Font And Language Guidance

- `Confirmed`: The product should be able to display non-ASCII metadata, including Chinese.
- `Adopt With Adaptation`: The exact font stack may change on Linux, but CJK-capable rendering is not optional.

## 12. Carry-Forward Decisions For LoFiBox Zero

### 12.1 What Must Survive

- offline local-player identity
- browse-first library structure
- smart playlists based on listening history
- cover-art-aware now playing experience
- dedicated equalizer as a first-class page, with the six-band legacy starting point preserved in the current implementation
- button-first interaction
- strong small-screen readability

### 12.2 What Must Be Reinterpreted

- fixed old display sizes
- old boot delay behavior
- old Arduino file, board, and UI framework structure
- fake status indicators
- old Cardputer keycode plumbing

### 12.3 What Can Stay Deferred

- editable user playlist creation
- advanced preset authoring and expanded DSP management beyond the current implemented EQ page
- broader format support beyond the first guaranteed starting set
- richer about-page branding polish
- explicit volume UX

## 13. Provisional And Design-Only Ideas

These ideas are worth keeping visible, but they are not current implementation requirements yet:

- custom named playlists with `Add Playlist...`
- richer `About` card with model number and branded hero art
- more polished main-menu iconography and shell-integrated mock presentation
- preset browsing beyond a static EQ preset label
- advanced DSP surfaces beyond the default EQ page

They should not be silently promoted into required scope without a deliberate project decision.

## 14. Implementation Consequences For This Repository

- Product concepts should live in `src/app` and related shared logic, not in platform adapters.
- Linux device specifics must continue to stay in `platform/device`.
- Physical shell and external harness presentation must remain outside shared app code.
- The app should target the current shared logical screen model of `320x170`, while preserving the product hierarchy described here.
- Design prototype images should be treated as external visual references that help translate the product into `320x170`, not as tracked runtime assets or pixel-perfect mandates.
- Imported runtime-candidate icons under `assets/ui/icons/legacy-lofibox` may seed future UI work, but the repository must not inherit the legacy code structure that originally used them.

## 15. Acceptance Heuristics For Future Features

Future feature work should be checked against this document with questions like:

- Does this strengthen offline handheld playback, or is it pulling the product toward a different category?
- Is this a real product object, or only a temporary implementation convenience?
- Does this remain usable with keyboard-only navigation?
- Does this preserve readability and low-friction use on `320x170`?
- Are we adding a truthful status or a fake decorative one?
- Are we preserving smart-playlist and metadata semantics instead of flattening everything into raw files?
- Are we translating old intent into Linux-native structure, or accidentally reviving the old architecture?

## 16. Summary Decision

The legacy `LoFiBox` repository is valid as a source of product meaning, visual direction, and interaction intent.
It is not valid as a source of architecture, framework shape, or Linux application structure.

For `LoFiBox Zero`, the correct carry-forward move is:

- inherit the offline player identity
- inherit the browse/play/EQ/smart-playlist product model
- inherit the small-screen visual hierarchy
- reinterpret everything through a Linux-native application architecture and a `320x170` display target
