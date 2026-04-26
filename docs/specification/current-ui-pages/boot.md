<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Current UI Page Spec: Boot

## 1. Purpose

- Establish product identity.
- Communicate that the media library is loading or becoming ready.

## 2. Related Specs

- Shared current UI rules: `docs/specification/lofibox-zero-page-spec.md`
- Library index state: `docs/specification/app-states/library-index-state.md`
- Layout template: `docs/specification/lofibox-zero-layout-spec.md`
- Visual styling: `docs/specification/lofibox-zero-visual-design-spec.md`

## 3. Entry Conditions

- `Boot` is the first UI surface shown when the application starts.
- No prior in-app navigation is required.

## 4. Data Dependencies

- product name or mark
- startup readiness status projected from library-index and persistence readiness
- loading message text
- optional real progress value when available

## 5. State Enumeration

- `STATE_BOOT_LOADING`
  startup prerequisites are not ready yet
- `STATE_BOOT_READY_TO_TRANSITION`
  minimum startup prerequisites are ready and Boot may hand off to `Main Menu`

## 6. Required Elements

- Product mark or text identity
- Loading message
- Optional progress indicator if real progress is available

## 7. Event Contract

- `EVT_PAGE_ENTER`
  - effect: enter `STATE_BOOT_LOADING`
- `EVT_PROGRESS_UPDATED`
  - effect: refresh visible progress if a real progress value exists
- `EVT_SYSTEM_READY`
  - effect: move Boot into `STATE_BOOT_READY_TO_TRANSITION` only when startup prerequisites and library readiness are truthfully satisfied

## 8. Transition Contract

- `STATE_BOOT_LOADING + EVT_PROGRESS_UPDATED` -> effect: update progress display if known -> `STATE_BOOT_LOADING`
- `STATE_BOOT_LOADING + EVT_SYSTEM_READY` -> effect: mark startup prerequisites ready -> `STATE_BOOT_READY_TO_TRANSITION`
- `STATE_BOOT_READY_TO_TRANSITION + EVT_SYSTEM_READY` -> effect: transition to `Main Menu` -> `Main Menu`

## 8.1 Page-Local F1 Help

- `Boot` is the only current page excluded from the `F1:HELP` contract.
- `Boot` `MUST NOT` open a help modal while startup animation, indexing bootstrap, or system readiness checks are in progress.

## 9. Empty State

- No dedicated empty state exists for `Boot`.

## 10. Error State

- No dedicated error screen belongs inside `Boot` in the current implementation.
- Boot `MUST NOT` invent fake percentages or fake diagnostics to simulate progress.

## 11. Non-Goals

- Detailed system diagnostics
- User settings
- Fake loading percentages
