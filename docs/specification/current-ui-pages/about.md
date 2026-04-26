<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Current UI Page Spec: About

## 1. Purpose

- Present concise product and device information.

## 2. Related Specs

- Shared current UI rules: `docs/specification/lofibox-zero-page-spec.md`
- Layout template: `docs/specification/lofibox-zero-layout-spec.md`
- Visual styling: `docs/specification/lofibox-zero-visual-design-spec.md`

## 3. Entry Conditions

- `About` is entered from `Settings`.

## 4. Data Dependencies

- product name
- software version
- storage usage or capacity summary
- optional device model
- optional simple branding asset or logo treatment

## 5. State Enumeration

- `STATE_ABOUT_READY`
  read-only information summary is visible

## 6. Required Elements

- Product name
- Software version
- Storage usage or capacity summary
- Device model when available
- Simple product branding or logo treatment when available

## 7. Event Contract

- `EVT_PAGE_ENTER`
  - effect: enter `STATE_ABOUT_READY`
- `EVT_BACK`
  - effect: return to `Settings`

## 8. Transition Contract

- `STATE_ABOUT_READY + EVT_BACK` -> effect: return to settings -> `Settings`

## 8.1 Page-Local F1 Help

- `F1` `MUST` open the `About` page's own help modal.
- The current `About` page has no explicit shortcut rows; therefore the modal `MUST` use the empty implementation.
- The modal `MUST NOT` reuse list, settings, or main-menu shortcut content.

## 9. Empty State

- No dedicated empty state exists for `About`.

## 10. Error State

- About `MUST NOT` require network access.
- Missing device information `MUST` degrade gracefully rather than showing placeholder fiction.
- The page `MUST NOT` remain only a raw two-row technical dump forever; even the current implementation should treat it as a small information summary page.

## 11. Non-Goals

- Legal or privacy documents
- Interactive maintenance tools
