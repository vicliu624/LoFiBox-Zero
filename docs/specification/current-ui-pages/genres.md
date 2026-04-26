<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Current UI Page Spec: Genres

## 1. Purpose

- Provide an alphabetical list of genres from the library.

## 2. Related Specs

- Shared current UI rules: `docs/specification/lofibox-zero-page-spec.md`
- Layout template: `docs/specification/lofibox-zero-layout-spec.md`

## 3. Entry Conditions

- `Genres` is entered from `Music Index`.

## 4. Data Dependencies

- genre names from the current media library
- current selected row

## 5. State Enumeration

- `STATE_GENRES_READY`
  genre list is non-empty and interactive
- `STATE_GENRES_EMPTY`
  no genres are available and the empty-state row is shown

## 6. Required Elements

- page title
- genre rows using genre name as primary text
- stable empty-state row when no genres exist

## 7. Event Contract

- `EVT_PAGE_ENTER`
  - effect: enter `STATE_GENRES_READY` when genres exist, otherwise `STATE_GENRES_EMPTY`
- `EVT_NAV_UP`
  - effect: move selection up when list is non-empty and selection is not first row
- `EVT_NAV_DOWN`
  - effect: move selection down when list is non-empty and selection is not last row
- `EVT_CONFIRM`
  - effect: open `Songs` in genre context when list is non-empty
- `EVT_BACK`
  - effect: return to `Music Index`

## 8. Transition Contract

- `STATE_GENRES_READY + EVT_NAV_UP [guard: selection is not first row]` -> effect: move selection up -> `STATE_GENRES_READY`
- `STATE_GENRES_READY + EVT_NAV_DOWN [guard: selection is not last row]` -> effect: move selection down -> `STATE_GENRES_READY`
- `STATE_GENRES_READY + EVT_CONFIRM` -> effect: open genre-context songs -> `Songs`
- `STATE_GENRES_READY + EVT_BACK` -> effect: return to library hub -> `Music Index`
- `STATE_GENRES_EMPTY + EVT_BACK` -> effect: return to library hub -> `Music Index`

## 8.1 Page-Local F1 Help

- `F1` `MUST` open the `Genres` page's own help modal.
- The current `Genres` page has no explicit shortcut rows; therefore the modal `MUST` use the empty implementation.
- The modal `MUST NOT` reuse Songs or Main Menu shortcut content.

## 9. Empty State

- The page `MUST` define a stable `No Genres` or equivalent empty state.

## 10. Error State

- No dedicated error page belongs here in the current implementation.
- Invalid genre data `MUST` degrade to the same stable empty-state behavior rather than broken rows.

## 11. Non-Goals

- Genre artwork
- Genre analytics
