<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Current UI Page Spec: Music Index

## 1. Purpose

- Provide category-based entry into the music library.

## 2. Related Specs

- Shared current UI rules: `docs/specification/lofibox-zero-page-spec.md`
- Library index state: `docs/specification/app-states/library-index-state.md`
- Layout template: `docs/specification/lofibox-zero-layout-spec.md`

## 3. Entry Conditions

- `Music Index` is entered from `Main Menu` by selecting `Library`.

## 4. Data Dependencies

- fixed category list for the current implementation
- library-index readiness truth
- current selected row

## 5. State Enumeration

- `STATE_MUSIC_INDEX_READY`
  library-category hub is interactive with one selected row

The current `Music Index` page `MUST` contain exactly these rows, in this order:

1. `Artists`
2. `Albums`
3. `Songs`
4. `Genres`
5. `Composers`
6. `Compilations`

## 6. Required Elements

- page title for the library browse hub
- six category rows
- visible row selection state

## 7. Event Contract

- `EVT_PAGE_ENTER`
  - effect: enter `STATE_MUSIC_INDEX_READY` and select the first actionable row
- `EVT_NAV_UP`
  - effect: move selection up when not already at the first row
- `EVT_NAV_DOWN`
  - effect: move selection down when not already at the last row
- `EVT_CONFIRM`
  - effect: open the selected destination
- `EVT_BACK`
  - effect: return to `Main Menu`

## 8. Transition Contract

- `STATE_MUSIC_INDEX_READY + EVT_NAV_UP [guard: selection is not first row]` -> effect: move selection up -> `STATE_MUSIC_INDEX_READY`
- `STATE_MUSIC_INDEX_READY + EVT_NAV_DOWN [guard: selection is not last row]` -> effect: move selection down -> `STATE_MUSIC_INDEX_READY`
- `STATE_MUSIC_INDEX_READY + EVT_CONFIRM [guard: selected row = Artists]` -> effect: open artists -> `Artists`
- `STATE_MUSIC_INDEX_READY + EVT_CONFIRM [guard: selected row = Albums]` -> effect: open all albums -> `Albums`
- `STATE_MUSIC_INDEX_READY + EVT_CONFIRM [guard: selected row = Songs]` -> effect: open all songs -> `Songs`
- `STATE_MUSIC_INDEX_READY + EVT_CONFIRM [guard: selected row = Genres]` -> effect: open genres -> `Genres`
- `STATE_MUSIC_INDEX_READY + EVT_CONFIRM [guard: selected row = Composers]` -> effect: open composers -> `Composers`
- `STATE_MUSIC_INDEX_READY + EVT_CONFIRM [guard: selected row = Compilations]` -> effect: open compilations -> `Compilations`
- `STATE_MUSIC_INDEX_READY + EVT_BACK` -> effect: return to main menu -> `Main Menu`

## 8.1 Page-Local F1 Help

- `F1` `MUST` open the `Music Index` page's own help modal.
- The current `Music Index` page has no explicit shortcut rows; therefore the modal `MUST` use the empty implementation.
- The modal `MUST NOT` reuse Songs or Main Menu shortcut content.

## 9. Empty State

- No dedicated empty state exists for `Music Index`; the page remains a fixed category hub even when the library has no content.

## 10. Error State

- The page `MUST NOT` fake dynamic library counts or invented rows.
- `Playlists` `MUST NOT` be added here merely because some design exports suggested it.

## 11. Non-Goals

- Search
- Sorting controls
- Nested settings or device info
