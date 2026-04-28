<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Layout Specification

## 1. Purpose

This document defines the page layout system for the `320x170` logical screen used by `LoFiBox Zero`.

It exists to stop page composition from drifting during implementation.
This document describes the current UI layout contract, not the product's final truth.

This document answers:

- which page template a screen uses
- where each element sits
- how much space each element owns
- how dense list pages may be

## 2. Canvas

- Logical canvas size: `320 x 170`
- Origin: top-left
- Coordinate system: integer pixels

## 3. Global Layout Templates

The current UI implementation uses six layout templates:

- `T0` Boot
- `T1` Main Menu
- `T2` List Shell
- `T3` Now Playing
- `T4` Equalizer
- `T5` About Card

## 4. Shared Metrics

### 4.1 Common Measurements

- `topbar-height`: `20`
- `screen-side-inset`: `4`
- `content-side-inset`: `6`
- `panel-radius`: visual only, geometry still uses the full logical bounds
- `divider-thickness`: `1`

### 4.2 Common Regions

- `topbar`: `x=0, y=0, w=320, h=20`
- `content-region`: `x=0, y=20, w=320, h=150`

### 4.3 Top Bar Zones

- `topbar-left-zone`: `x=6, y=0, w=78, h=20`
- `topbar-title-zone`: `x=84, y=0, w=152, h=20`
- `topbar-status-zone`: `x=236, y=0, w=78, h=20`

These are ownership zones, not necessarily literal widget bounding boxes.

The standard top bar left zone `MUST` render `F1:HELP` on every non-boot page that uses the standard top bar.
Back-navigation state belongs to input handling and the navigation stack; it `MUST NOT` replace the left-zone help label with a raw back glyph.
All textual content in the top bar zones `MUST` be vertically centered within the `20` pixel topbar, including the left help label, centered title, right status/source label, and the Main Menu playback summary strip.

## 5. Template T0: Boot

### Geometry

- `brand-block`: `x=32, y=34, w=256, h=54`
- `loading-label`: `x=80, y=104, w=160, h=16`
- `progress-region`: `x=64, y=128, w=192, h=10`

### Rules

- Boot content `MUST` be vertically centered as a group.
- If no real progress exists, `progress-region` may be omitted and the loading label may shift down slightly.

## 6. Template T1: Main Menu

### Geometry

- `top-strip`: `x=6, y=6, w=308, h=18`
- `status-strip`: `x=254, y=6, w=58, h=10`
- `left-arrow-zone`: reserved legacy coordinate only; current Main Menu `MUST NOT` require a visible arrow here
- `right-arrow-zone`: reserved legacy coordinate only; current Main Menu `MUST NOT` require a visible arrow here
- `selected-card`: `x=112, y=24, w=96, h=96`
- `left-preview-card`: `x=22, y=38, w=68, h=68`
- `right-preview-card`: `x=230, y=38, w=68, h=68`
- `selected-label`: `x=108, y=124, w=104, h=18`
- `side-label-left`: `x=18, y=120, w=96, h=14`
- `side-label-right`: `x=206, y=120, w=96, h=14`
- `pagination-dots`: `x=132, y=150, w=56, h=10`

### Rules

- `selected-card` is mandatory.
- Side preview cards are mandatory when adjacent destinations exist.
- `left-arrow-zone` and `right-arrow-zone` are not mandatory current visuals; circular side preview cards carry the navigation affordance.
- `selected-label` is mandatory.
- Side labels are mandatory when their corresponding preview cards are visible.
- `pagination-dots` are mandatory.
- `status-strip` is mandatory.
- `top-strip` is mandatory.
- The main menu `MUST NOT` use the standard top bar template.
- The main menu `MUST NOT` draw an additional boxed stage frame around the carousel content.

## 7. Template T2: List Shell

This template is used by:

- `Music Index`
- `Artists`
- `Albums`
- `Songs`
- `Genres`
- `Composers`
- `Compilations`
- `Playlists`
- `Playlist Detail`
- `Search`
- `Settings`
- `Remote Setup`
- `Remote Profile Settings`
- `Remote Field Editor`

### Geometry

- `topbar`: common top bar
- `list-frame`: `x=6, y=26, w=308, h=132`
- `row-height`: `22`
- `visible-row-count`: `6`

### Row Slots

The six row slots are:

1. `x=8, y=26, w=304, h=22`
2. `x=8, y=48, w=304, h=22`
3. `x=8, y=70, w=304, h=22`
4. `x=8, y=92, w=304, h=22`
5. `x=8, y=114, w=304, h=22`
6. `x=8, y=136, w=304, h=22`

### Common Row Zones

- `row-icon-zone`: `x=12..28`
- `row-primary-zone-no-icon`: starts at `x=12`
- `row-primary-zone-with-icon`: starts at `x=34`
- `row-right-zone`: ends at `x=300`

The row-right zone is reserved for:

- value text
- artist secondary text
- chevron affordance

### T2 Variants

#### T2-A Simple Browse Row

Used by:

- `Music Index`
- `Artists`
- `Genres`
- `Composers`
- `Compilations`
- `Playlists`

Zones:

- `primary`: `x=12, y=row+3, w=252, h=16`
- `chevron`: `x=286, y=row+3, w=14, h=16`

#### T2-B Detail Browse Row

Used by:

- `Albums`
- `Playlist Detail`

Zones:

- `primary`: `x=12, y=row+3, w=182, h=16`
- `secondary-right`: `x=198, y=row+3, w=88, h=16`
- `chevron-optional`: `MUST NOT` be shown if it compromises secondary text readability

#### T2-C Song Row

Used by:

- `Songs`

Zones:

- `primary`: `x=12, y=row+3, w=276, h=16`
- no right metadata in the current implementation

#### T2-D Settings Row

Used by:

- `Settings`

Zones:

- `icon`: `x=12, y=row+3, w=16, h=16`
- `primary`: `x=36, y=row+3, w=160, h=16`
- `value-or-chevron`: `x=212, y=row+3, w=88, h=16`

#### T2-E Search Row

Used by:

- `Search`

Search keeps the standard list shell but reserves the first row for query entry.
When preedit or source-status text is present, the second row may become a non-actionable status row.
Result rows then occupy the remaining visible slots.

Zones:

- `query-label`: first row `x=12, y=row+3, w=50, h=16`
- `query-text`: first row `x=66, y=row+3, w=232, h=16`
- `preedit-or-status`: optional second row `x=12, y=row+3, w=286, h=16`
- `result-primary`: result rows `x=12, y=row+3, w=188, h=16`
- `result-source`: result rows `x=206, y=row+3, w=92, h=16`

The query and preedit/status rows are not normal playable result rows.
Selection movement must wrap across actionable result rows without treating the query prompt as a playable item.

#### T2-F Field Editor Row

Used by:

- `Remote Field Editor`

Field editor pages keep a compact list shell but reserve the first rows for one editable value rather than for browse results.

Zones:

- `field-label`: first row `x=12, y=row+3, w=286, h=16`
- `field-value`: second row `x=12, y=row+3, w=286, h=16`
- `preedit-or-validation`: optional third row `x=12, y=row+3, w=286, h=16`
- `action-primary`: action rows `x=12, y=row+3, w=188, h=16`
- `action-status`: action rows `x=206, y=row+3, w=92, h=16`

Secret values must reserve the same value geometry but render redacted text.
Validation status must not resize the row layout.

### Rules

- Browse pages `MUST NOT` exceed six visible rows in the current layout contract.
- Scrolling `MUST` happen by moving the windowed row set, not by shrinking text to fit more rows.
- Pages sharing `T2` `MUST` keep the same row cadence.

## 8. Template T3: Now Playing

### Geometry

- `topbar`: common top bar
- `cover`: `x=8, y=32, w=96, h=96`
- `title`: `x=116, y=30, w=196, h=20`
- `artist`: `x=116, y=52, w=196, h=14`
- `album`: `x=116, y=68, w=196, h=14`
- `progress-track`: `x=116, y=92, w=184, h=6`
- `elapsed-time`: `x=116, y=102, w=34, h=12`
- `total-time`: `x=266, y=102, w=34, h=12`
- `transport-row`: `x=108, y=122, w=204, h=24`

### Transport Slots

Five evenly distributed slots inside `transport-row`:

1. previous
2. play/pause
3. next
4. shuffle
5. repeat

### Rules

- Cover `MUST` remain square.
- Metadata `MUST` remain to the right of cover in the current layout contract.
- Progress bar `MUST` sit below metadata and above controls.
- Transport row `MUST` sit on a single line in the current layout contract.
- The page `MUST NOT` push controls under the cover in the current layout contract.

## 9. Template T4: Equalizer

### Geometry

- `topbar`: common top bar
- `eq-panel`: `x=6, y=24, w=308, h=140`
- `left-label-column`: `x=10, y=34, w=42, h=86`
- `db-scale-column`: `x=50, y=44, w=24, h=56`
- `band-gain-row`: `x=78, y=30, w=222, h=12`
- `slider-graph-region`: `x=78, y=44, w=222, h=56`
- `band-label-row`: `x=78, y=118, w=222, h=16`
- `preset-strip`: `x=8, y=142, w=304, h=16`

### Slider Geometry

- visible slider count: `10`
- each slider column width: `18`
- inter-column gap: `4`
- slider body height: `54`
- slider body vertical center line aligned to `0 dB`

### Rules

- The EQ graph region `MUST` visually dominate the page.
- Left labels and dB labels `MUST` remain outside the slider area.
- The per-band gain row `MUST` show each band's current signed gain value above its slider column.
- Preset strip `MUST` remain below the graph region.
- The layout `MUST NOT` collapse into a list-page presentation.

## 10. Template T5: About Card

### Geometry

- `topbar`: common top bar
- `about-card`: `x=8, y=30, w=304, h=126`
- `brand-block`: `x=16, y=40, w=88, h=76`
- `info-block`: `x=120, y=38, w=176, h=54`
- `divider`: `x=120, y=98, w=168, h=1`
- `footer-info`: `x=120, y=108, w=168, h=18`

### Rules

- `brand-block` is reserved for logo/artwork plus product name.
- `info-block` must fit at least three compact rows:
  - version
  - storage
  - optional model
- `footer-info` may hold a short product subtitle or project line.
- The page `MUST NOT` use the generic six-row list template in the current layout contract.

## 11. Page-To-Template Mapping

| Page | Template | Notes |
| --- | --- | --- |
| Boot | `T0` | No top bar |
| Main Menu | `T1` | No standard top bar |
| Music Index | `T2-A` | simple browse rows |
| Artists | `T2-A` | simple browse rows |
| Albums | `T2-B` | primary plus album artist |
| Songs | `T2-C` | title-focused track rows |
| Genres | `T2-A` | simple browse rows |
| Composers | `T2-A` | simple browse rows |
| Compilations | `T2-A` | simple browse rows |
| Playlists | `T2-A` | simple browse rows |
| Playlist Detail | `T2-B` | title plus artist |
| Search | `T2-E` | pinned query row plus source-aware results |
| Now Playing | `T3` | dedicated playback layout |
| Equalizer | `T4` | dedicated EQ layout |
| Settings | `T2-D` | icon plus value/chevron |
| Remote Setup | `T2-A` | supported remote source kinds before profile fields |
| Remote Profile Settings | `T2-B` | concrete selected-kind profile fields |
| Remote Field Editor | `T2-F` | one editable remote profile field |
| About | `T5` | dedicated information card |

## 12. Layout Prohibitions

The current layout system `MUST NOT` do these things:

- use full-screen flat lists without top-bar structure on non-root pages
- squeeze more than six browse rows onto `320x170`
- move `Now Playing` controls into multiple stacked rows in the current layout contract
- present `Equalizer` as a standard settings list
- leave `About` as a raw list-page clone
- invent arbitrary per-page paddings outside this system

## 13. Responsive Adaptation Inside 320x170

`LoFiBox Zero` is not responsive in the web sense.
However, these minor adaptations are allowed inside the fixed `320x170` screen:

- hide side labels on `Main Menu` if they become illegible
- truncate long secondary text on detail rows
- clip or scroll `Now Playing` title only when required
- omit optional model row on `About` when data is unavailable

These adaptations `MUST NOT` change template ownership or page purpose.

## 14. Change Rule

If a page implementation requires geometry outside this document:

- update this layout spec first
- then implement the code change

Do not solve layout pressure with silent local exceptions.
