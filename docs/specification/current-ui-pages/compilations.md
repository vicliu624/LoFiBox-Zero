<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Current UI Page Spec: Compilations

## 1. Purpose

- Provide access to albums that represent multi-artist compilations.

## 2. Related Specs

- Shared current UI rules: `docs/specification/lofibox-zero-page-spec.md`
- Layout template: `docs/specification/lofibox-zero-layout-spec.md`
- Library interpretation: `docs/specification/legacy-lofibox-product-guidance.md`

## 3. Entry Conditions

- `Compilations` is entered from `Music Index`.

## 4. Data Dependencies

- compilation-album set derived from scanned library data
- current selected row

## 5. State Enumeration

- `STATE_COMPILATIONS_READY`
  compilation list is non-empty and interactive
- `STATE_COMPILATIONS_EMPTY`
  no compilations are available and the empty-state row is shown

## 6. Required Elements

- page title
- compilation rows using album name as primary text
- stable empty-state row when no compilations exist

## 7. Event Contract

- `EVT_PAGE_ENTER`
  - effect: enter `STATE_COMPILATIONS_READY` when compilations exist, otherwise `STATE_COMPILATIONS_EMPTY`
- `EVT_NAV_UP`
  - effect: move selection up when list is non-empty and selection is not first row
- `EVT_NAV_DOWN`
  - effect: move selection down when list is non-empty and selection is not last row
- `EVT_CONFIRM`
  - effect: open `Songs` in compilation-album context when list is non-empty
- `EVT_BACK`
  - effect: return to `Music Index`

## 8. Transition Contract

- `STATE_COMPILATIONS_READY + EVT_NAV_UP [guard: selection is not first row]` -> effect: move selection up -> `STATE_COMPILATIONS_READY`
- `STATE_COMPILATIONS_READY + EVT_NAV_DOWN [guard: selection is not last row]` -> effect: move selection down -> `STATE_COMPILATIONS_READY`
- `STATE_COMPILATIONS_READY + EVT_CONFIRM` -> effect: open compilation-context songs -> `Songs`
- `STATE_COMPILATIONS_READY + EVT_BACK` -> effect: return to library hub -> `Music Index`
- `STATE_COMPILATIONS_EMPTY + EVT_BACK` -> effect: return to library hub -> `Music Index`

## 8.1 Page-Local F1 Help

- `F1` `MUST` open the `Compilations` page's own help modal.
- The current `Compilations` page has no explicit shortcut rows; therefore the modal `MUST` use the empty implementation.
- The modal `MUST NOT` reuse Songs or Main Menu shortcut content.

## 9. Empty State

- The page `MUST` define a stable `No Compilations` or equivalent empty state.

## 10. Error State

- No dedicated error page belongs here in the current implementation.
- The implementation `MUST NOT` treat compilation browsing as if it were identical to single-artist album browsing.
- Invalid compilation derivation `MUST` degrade to the stable empty-state behavior rather than mixed or duplicated rows.

## 11. Non-Goals

- Dedicated compilation-artists page
- Compilation editing or tagging UI
