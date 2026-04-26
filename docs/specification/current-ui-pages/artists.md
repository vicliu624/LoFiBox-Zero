<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Current UI Page Spec: Artists

## 1. Purpose

- Provide an alphabetical browse list of unique artists.

## 2. Related Specs

- Shared current UI rules: `docs/specification/lofibox-zero-page-spec.md`
- Library index state: `docs/specification/app-states/library-index-state.md`
- Layout template: `docs/specification/lofibox-zero-layout-spec.md`
- Metadata rules: `docs/specification/legacy-lofibox-product-guidance.md`

## 3. Entry Conditions

- `Artists` is entered from `Music Index`.

## 4. Data Dependencies

- unique artist names projected from the current library index
- current selected row

## 5. State Enumeration

- `STATE_ARTISTS_LIST_READY`
  artist list is non-empty and interactive
- `STATE_ARTISTS_EMPTY`
  no artists are available and the empty-state row is shown

## 6. Required Elements

- page title
- artist rows using artist name as primary text
- stable empty-state row when no artists exist

## 7. Event Contract

- `EVT_PAGE_ENTER`
  - effect: enter `STATE_ARTISTS_LIST_READY` when artists exist, otherwise enter `STATE_ARTISTS_EMPTY`
- `EVT_NAV_UP`
  - effect: move selection up when list is non-empty and selection is not first row
- `EVT_NAV_DOWN`
  - effect: move selection down when list is non-empty and selection is not last row
- `EVT_CONFIRM`
  - effect: open `Albums` filtered to the selected artist when list is non-empty
- `EVT_BACK`
  - effect: return to `Music Index`

## 8. Transition Contract

- `STATE_ARTISTS_LIST_READY + EVT_NAV_UP [guard: selection is not first row]` -> effect: move selection up -> `STATE_ARTISTS_LIST_READY`
- `STATE_ARTISTS_LIST_READY + EVT_NAV_DOWN [guard: selection is not last row]` -> effect: move selection down -> `STATE_ARTISTS_LIST_READY`
- `STATE_ARTISTS_LIST_READY + EVT_CONFIRM` -> effect: open artist-filtered albums -> `Albums`
- `STATE_ARTISTS_LIST_READY + EVT_BACK` -> effect: return to library hub -> `Music Index`
- `STATE_ARTISTS_EMPTY + EVT_BACK` -> effect: return to library hub -> `Music Index`

## 8.1 Page-Local F1 Help

- `F1` `MUST` open the `Artists` page's own help modal.
- The current `Artists` page has no explicit shortcut rows; therefore the modal `MUST` use the empty implementation.
- The modal `MUST NOT` reuse Songs or Main Menu shortcut content.

## 9. Empty State

- If there are no artists, the page `MUST` show a stable non-interactive empty state such as `No Music`.

## 10. Error State

- No dedicated error page belongs here in the current implementation.
- Missing or invalid artist data `MUST` degrade to the same stable empty-state behavior rather than broken rows or invented placeholder artists.

## 11. Non-Goals

- Artist artwork
- Track counts in the current implementation
- Artist biography or metadata pages
