<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Current UI Page Spec: Albums

## 1. Purpose

- Provide album browsing either globally or within a selected artist.

## 2. Related Specs

- Shared current UI rules: `docs/specification/lofibox-zero-page-spec.md`
- Layout template: `docs/specification/lofibox-zero-layout-spec.md`

## 3. Entry Conditions

- `Albums` is entered either from `Music Index` or from `Artists`.

## 4. Data Dependencies

- all-albums list or artist-filtered album list
- album name
- album artist
- current selected row
- current mode origin

## 5. State Enumeration

- `STATE_ALBUMS_ALL_READY`
  all-albums mode is interactive
- `STATE_ALBUMS_BY_ARTIST_READY`
  artist-filtered mode is interactive
- `STATE_ALBUMS_EMPTY`
  current album context is empty

## 6. Required Elements

- page title reflecting current mode
- album rows with primary album name and secondary album artist
- stable empty-state row when the current context contains no albums

## 7. Event Contract

- `EVT_PAGE_ENTER`
  - effect: enter `STATE_ALBUMS_ALL_READY`, `STATE_ALBUMS_BY_ARTIST_READY`, or `STATE_ALBUMS_EMPTY` based on origin and available data
- `EVT_NAV_UP`
  - effect: move selection up when current album list is non-empty and selection is not first row
- `EVT_NAV_DOWN`
  - effect: move selection down when current album list is non-empty and selection is not last row
- `EVT_CONFIRM`
  - effect: open `Songs` in album context when current album list is non-empty
- `EVT_BACK`
  - effect: return to the page that established the current mode

## 8. Transition Contract

- `STATE_ALBUMS_ALL_READY + EVT_NAV_UP [guard: selection is not first row]` -> effect: move selection up -> `STATE_ALBUMS_ALL_READY`
- `STATE_ALBUMS_ALL_READY + EVT_NAV_DOWN [guard: selection is not last row]` -> effect: move selection down -> `STATE_ALBUMS_ALL_READY`
- `STATE_ALBUMS_ALL_READY + EVT_CONFIRM` -> effect: open album-context songs -> `Songs`
- `STATE_ALBUMS_ALL_READY + EVT_BACK` -> effect: return to `Music Index` -> `Music Index`
- `STATE_ALBUMS_BY_ARTIST_READY + EVT_NAV_UP [guard: selection is not first row]` -> effect: move selection up -> `STATE_ALBUMS_BY_ARTIST_READY`
- `STATE_ALBUMS_BY_ARTIST_READY + EVT_NAV_DOWN [guard: selection is not last row]` -> effect: move selection down -> `STATE_ALBUMS_BY_ARTIST_READY`
- `STATE_ALBUMS_BY_ARTIST_READY + EVT_CONFIRM` -> effect: open album-context songs -> `Songs`
- `STATE_ALBUMS_BY_ARTIST_READY + EVT_BACK` -> effect: return to `Artists` -> `Artists`
- `STATE_ALBUMS_EMPTY + EVT_BACK [guard: origin = Music Index]` -> effect: return to library hub -> `Music Index`
- `STATE_ALBUMS_EMPTY + EVT_BACK [guard: origin = Artists]` -> effect: return to artists list -> `Artists`

## 8.1 Page-Local F1 Help

- `F1` `MUST` open the `Albums` page's own help modal.
- The current `Albums` page has no explicit shortcut rows; therefore the modal `MUST` use the empty implementation.
- The modal `MUST NOT` reuse Songs or Main Menu shortcut content.

## 9. Empty State

- If there are no albums in the current context, the page `MUST` show `No Albums` or equivalent.

## 10. Error State

- No dedicated error page belongs here in the current implementation.
- Invalid album metadata `MUST NOT` produce broken row structure; it must degrade to stable album rows or the same empty-state behavior.

## 11. Non-Goals

- Album details page separate from track listing
- Grid album-wall in the current implementation
