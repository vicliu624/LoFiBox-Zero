<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Visual Design Specification

## 1. Purpose

This document defines the visual design system for `LoFiBox Zero`.

Its job is to constrain how UI elements look so that implementation does not drift page by page.

This document does not decide whether a control belongs in the final product.
Current UI control and page presence belong to `lofibox-zero-page-spec.md`.
Final product meaning belongs to `lofibox-zero-final-product-spec.md`.

## 2. Design Intent

`LoFiBox Zero` should look like a focused handheld music device:

- dark
- high-contrast
- compact
- playback-first
- crisp on a small display

The product should feel deliberate and device-like, not like a desktop app miniaturized onto `320x170`.

## 3. Global Visual Principles

### 3.1 Tone

- The UI `MUST` feel calm and low-distraction.
- The UI `MUST` prioritize readability over decoration.
- The UI `SHOULD` feel slightly glossy and media-device-like rather than flat and generic.

### 3.2 Truthfulness

- Decorative status icons `MUST NOT` appear unless backed by real data.
- Empty states `MUST NOT` use fake content.
- Device chrome `MUST NOT` be drawn inside app content unless it is actually part of the content.

### 3.3 Small-Screen Discipline

- Visual hierarchy `MUST` be obvious within one glance.
- The page `MUST` avoid cluttered micro-labels.
- Secondary text `MUST` be visually weaker than primary text.
- Decorative shadows and glows `MUST` remain restrained.

## 4. Color System

These tokens are the current UI visual palette.

### 4.1 Neutral Tokens

- `bg-root`: `#050608`
- `bg-panel-0`: `#0A0C0F`
- `bg-panel-1`: `#101318`
- `bg-panel-2`: `#171B21`
- `chrome-topbar-0`: `#2B2E33`
- `chrome-topbar-1`: `#101317`
- `divider`: `#2A2F36`
- `text-primary`: `#F5F7FA`
- `text-secondary`: `#C7CDD3`
- `text-muted`: `#8D949C`
- `text-disabled`: `#5F6670`

### 4.2 Focus And Action Tokens

- `focus-fill-0`: `#5FB0FF`
- `focus-fill-1`: `#2F86FF`
- `focus-edge`: `#A9DBFF`
- `accent-progress`: `#58A8FF`
- `accent-progress-strong`: `#2F75FF`

### 4.3 Equalizer Tokens

- `eq-hot-0`: `#FFB257`
- `eq-hot-1`: `#FF7F2A`
- `eq-hot-edge`: `#FFE0B0`
- `eq-selected-0`: `#7FD1FF`
- `eq-selected-1`: `#2F86FF`

### 4.4 Informational Tokens

- `battery-good`: `#87D96C`
- `battery-warn`: `#E6B34A`
- `battery-bad`: `#D66A5F`

### 4.5 Explicit Constraint

- Blue is the current UI focus color for navigation and progress.
- Orange is reserved for EQ emphasis and should not become the global focus color.
- Purple and neon pink may appear inside imported artwork or icon imagery, but they `MUST NOT` become current shell chrome colors.

## 5. Typography

### 5.1 Font Roles

The UI `MUST` support CJK-safe rendering.

The current visual roles are:

- `display-title`
  16 px equivalent, medium or semibold
- `page-title`
  14 px to 16 px equivalent, medium
- `row-primary`
  14 px to 15 px equivalent, medium
- `row-secondary`
  11 px to 12 px equivalent, regular
- `meta`
  10 px to 12 px equivalent, regular
- `status`
  10 px to 11 px equivalent, regular

### 5.2 Typography Rules

- Primary labels `MUST` use `text-primary`.
- Secondary metadata `MUST` use `text-secondary` or `text-muted`.
- Disabled or unavailable text `MUST` use `text-disabled`.
- Titles `MUST NOT` be visually smaller than row labels.

### 5.3 Scrolling Text

- Continuous scrolling text `MAY` be used for `Now Playing` title only when needed.
- Scrolling text `MUST NOT` become the default treatment for ordinary list rows.

## 6. Chrome Components

## 6.1 Top Bar

### Visual Contract

- The top bar `MUST` use a dark metallic or glossy gradient treatment.
- It `MUST` visually separate itself from content with a divider or tonal break.
- The top bar `MUST` have three functional zones:
  - left action/context
  - centered page title
  - right status cluster

### Content Rules

- Standard top bars `MUST` show `F1:HELP` in the left zone on every non-boot page that uses the standard top bar.
- Back navigation belongs to keyboard behavior and page-local help; it must not replace the left-zone help label with a raw back glyph.
- Title zone `MUST` remain visually centered.
- Right zone `MAY` contain battery and other truthful device status.

### Prohibitions

- No fake Wi-Fi.
- No fake signal bars.
- No fake time display unless system time is real and required by product decision.

## 6.2 List Row

### Visual Contract

- Rows `MUST` be clean and dense.
- Unselected rows `MUST` sit on a dark background.
- Selected rows `MUST` use a translucent blue glass-focus treatment, not a hard opaque rectangle.
- Selected-row focus `MUST` be strongest near the vertical center and fade toward the top and bottom edges.
- Selected-row focus `MUST` include only subtle crystal highlights and `MUST NOT` use a heavy hard border.
- Primary text on a selected row `MUST` remain high-contrast white or near-white.

### Row Variants

- `simple-row`
  primary text plus chevron
- `detail-row`
  primary text plus right-side secondary text
- `icon-row`
  leading icon plus primary text plus right-side value or chevron

### Divider Rule

- Dividers `SHOULD` be subtle and dark.
- Dividers `MUST NOT` overpower row text.

### Scrollbar Rule

- Browse-list scrollbars `MUST` use the same translucent crystal visual language as selected-row focus.
- Scrollbar tracks `MUST` be subdued and semi-transparent rather than hard solid rails.
- Scrollbar thumbs `MUST` fade at their top and bottom edges and use a soft blue inner highlight.
- Scrollbar thumbs `MUST NOT` be rendered as hard opaque blue rectangles.

### Shortcut Hint Rule

- Browse-list top bars `MUST` use `F1:HELP` in the left zone instead of a back arrow.
- The help hint `MUST` be visible but quieter than the page title.
- The shortcut-help modal `MUST` appear as a modal overlay and `MUST NOT` look like another list row.

## 6.3 Empty State Row

- Empty states on list pages `MUST` look inactive.
- They `MUST NOT` use the selected-row fill by default.
- They `SHOULD` use muted text.

## 6.4 Main Menu Card

### Visual Contract

- Main menu cards `MUST` feel more iconic and branded than list rows.
- The selected card `MUST` be larger and visually dominant.
- Side preview cards `MUST` be visibly subordinate.
- Side preview cards `MUST` always be populated through circular carousel wrap; first and last entries are adjacent.
- Pagination dots `MUST` reinforce position without competing with the selected card.
- The main menu `MUST NOT` expose disabled start/end affordances; there is no visual terminal edge.
- A compact top-right status strip `MUST` be present.

### Icon Treatment

- Visible main-menu cards `MUST` contain destination-specific iconography.
- Imported legacy icon art `MAY` inform this page, but equivalent runtime-drawn iconography is acceptable.
- Blank, iconless color blocks `MUST NOT` be used as final card rendering.
- Main menu card chrome `MUST NOT` become visually noisy or toy-like.

## 6.4.1 Product Logo Transparency

- The formal LoFiBox application logo is a governed product asset, not a page
  background.
- Runtime and installable logo projections `MUST` preserve real alpha
  transparency. Flattened white backgrounds, off-white export mattes, and
  checkerboard transparency previews are invalid logo assets.
- Corner pixels `SHOULD` be fully transparent; near-transparent antialiasing at
  the extreme edge is acceptable when it is visually invisible on the boot
  background.
- Startup, About, desktop metadata, hicolor icon, and documentation projections
  `MUST` derive from the same transparent logo source semantics.
- Rendering code `MUST` alpha-composite the logo over the current page
  background; it `MUST NOT` make logo transparency dependent on the boot page
  using a matching solid background color.
- A smoke test `MUST` verify that the runtime logo asset has transparent
  corners and that scaled logo blitting preserves the destination background
  behind transparent pixels.

## 6.5 Progress Bar

- The current progress bar `MUST` use a cool blue fill.
- The track behind it `MUST` remain visible but subdued.
- The knob or head marker `SHOULD` be light-colored and easy to locate.

## 6.6 Transport Controls

- Transport icons `MUST` be visually clear at a glance.
- The active play/pause state `MUST` be more visually important than shuffle or repeat.
- Shuffle and repeat `SHOULD` use active-state tinting rather than extra labels.

## 6.7 Equalizer Slider

- EQ sliders `MUST` feel tactile and more expressive than ordinary list controls.
- Warm orange is the current EQ slider color.
- The selected EQ slider `MUST` receive a blue-tinted selection treatment that is visually distinct from its default orange fill.
- The `0 dB` line `MUST` be more prominent than other grid lines.
- EQ slider tracks and fills `MUST` use the same glass/gradient language as focused rows, progress bars, and the Main Menu chrome; flat opaque rectangular bars are not acceptable current styling.
- Each EQ slider column `MUST` show the band's current signed gain above the slider so users can read the exact value without inferring it from bar height.

## 6.8 About Card

- About `SHOULD` feel like a compact product information card rather than a raw table dump.
- Branding or logo treatment `MAY` appear on the left.
- Information rows on the right `MUST` remain readable and restrained.

## 6.9 Text Input And Search Rows

- Text-entry rows `MUST` visually distinguish committed text from transient preedit text.
- Committed query text should use primary or strong secondary text depending on available space.
- Placeholder prompts such as `TYPE...` must look inactive and must not look like search results.
- Preedit text, when shown, should be visually adjacent to the query while remaining clearly temporary.
- Search result source labels must use muted or secondary text and must not overpower track, album, artist, or station names.
- Invalid or unsupported text-input states must degrade as status text rather than corrupting the visible query.

## 7. Page Family Styles

## 7.1 Root Pages

- `Main Menu` is the only current implementation page that may visually depart from the standard top-bar-plus-list shell.
- It `SHOULD` feel like a front door, not just another list.

## 7.2 Browse Pages

Pages:

- `Music Index`
- `Artists`
- `Albums`
- `Songs`
- `Genres`
- `Composers`
- `Compilations`
- `Playlists`
- `Playlist Detail`
- `Settings`
- `Search`
- `Remote Setup`
- `Remote Profile Settings`
- `Remote Field Editor`

These pages `MUST` share a common family resemblance:

- same top bar
- same dark content field
- same row focus treatment
- same density discipline

## 7.3 Playback Page

`Now Playing` `MUST` look more content-rich than browse pages while staying readable.

It `SHOULD` emphasize:

- cover
- title
- progress
- transport

It `MUST NOT` look like a list page with bolted-on controls.

## 7.4 Equalizer Page

`Equalizer` `MUST` visually stand apart from ordinary settings and browse pages.

It `SHOULD` feel like an instrument panel:

- graph-led
- tactile
- dense but not cluttered

## 7.5 Information Page

`About` `SHOULD` feel calmer and more informational than `Settings`.

## 8. Device Surface Boundary

- App pages `MUST` be designed as if they own the full `320x170` logical screen.
- Physical enclosure, external test harness, or container presentation details `MUST NOT` become part of app content styling.
- The app visual system must stay self-consistent on the logical screen without depending on shell chrome.

## 9. Visual Prohibitions

The current UI `MUST NOT` do these things:

- invent decorative status icons that are not real
- use unrelated per-page color palettes
- apply large desktop-app paddings that waste `320x170`
- let every page choose its own highlight color
- turn text into tiny low-contrast microcopy
- mix physical shell or harness ornament into page content
- overuse glow effects or heavy shadows

## 10. Change Rule

If implementation needs a new visual pattern:

- update this document first
- then implement the new pattern in code

Do not solve visual inconsistency page by page with local exceptions.
