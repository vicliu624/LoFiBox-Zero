<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Current UI Page Spec: Playlists

## 1. Purpose

- Provide access to the currently implemented smart playlists.

## 2. Related Specs

- Shared current UI rules: `docs/specification/lofibox-zero-page-spec.md`
- Library index state: `docs/specification/app-states/library-index-state.md`
- Layout template: `docs/specification/lofibox-zero-layout-spec.md`
- Smart-playlist semantics: `docs/specification/legacy-lofibox-product-guidance.md`

## 3. Entry Conditions

- `Playlists` is entered from `Main Menu`.

## 4. Data Dependencies

- fixed smart-playlist definitions
- listening-history and library-index readiness truth
- current selected row

## 5. State Enumeration

- `STATE_PLAYLISTS_READY`
  smart-playlist menu is interactive

The current `Playlists` page `MUST` contain exactly these rows, in this order:

1. `On-The-Go`
2. `Recently Added`
3. `Most Played`
4. `Recently Played`

## 6. Required Elements

- page title
- four smart-playlist rows
- visible selection state

## 7. Event Contract

- `EVT_PAGE_ENTER`
  - effect: enter `STATE_PLAYLISTS_READY` and select the first row
- `EVT_NAV_UP`
  - effect: move selection up when selection is not first row
- `EVT_NAV_DOWN`
  - effect: move selection down when selection is not last row
- `EVT_CONFIRM`
  - effect: open `Playlist Detail`
- `EVT_BACK`
  - effect: return to `Main Menu`

## 8. Transition Contract

- `STATE_PLAYLISTS_READY + EVT_NAV_UP [guard: selection is not first row]` -> effect: move selection up -> `STATE_PLAYLISTS_READY`
- `STATE_PLAYLISTS_READY + EVT_NAV_DOWN [guard: selection is not last row]` -> effect: move selection down -> `STATE_PLAYLISTS_READY`
- `STATE_PLAYLISTS_READY + EVT_CONFIRM` -> effect: open selected playlist -> `Playlist Detail`
- `STATE_PLAYLISTS_READY + EVT_BACK` -> effect: return to main menu -> `Main Menu`

## 8.1 Page-Local F1 Help

- `F1` `MUST` open the `Playlists` page's own help modal.
- The current `Playlists` page has no explicit shortcut rows; therefore the modal `MUST` use the empty implementation.
- The modal `MUST NOT` reuse Songs or Main Menu shortcut content.

## 9. Empty State

- No dedicated empty state exists for `Playlists`; the page remains a fixed playlist menu even when the selected playlist would later be empty.

## 10. Error State

- No dedicated error page belongs here in the current implementation.
- Custom user-authored playlists `MUST NOT` be faked into this page.
- `Add Playlist...` `MUST NOT` appear merely because it exists in design exports.

## 11. Non-Goals

- Playlist creation
- Playlist rename/delete/edit UI
