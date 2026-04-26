<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Current UI Page Spec: Equalizer

## 1. Purpose

- Provide a dedicated ten-band EQ tuning page for the current implementation.

## 2. Related Specs

- Shared current UI rules: `docs/specification/lofibox-zero-page-spec.md`
- Layout template: `docs/specification/lofibox-zero-layout-spec.md`
- Visual styling: `docs/specification/lofibox-zero-visual-design-spec.md`
- DSP capability boundary: `docs/specification/lofibox-zero-audio-dsp-spec.md`

## 3. Entry Conditions

- `Equalizer` is entered from `Main Menu`.

## 4. Data Dependencies

- ten visible EQ band values
- current selected band
- current preset display value

## 5. State Enumeration

- `STATE_EQUALIZER_READY`
  equalizer page is interactive with one selected visible band

Visible bands `MUST` be:

1. `31 Hz`
2. `62 Hz`
3. `125 Hz`
4. `250 Hz`
5. `500 Hz`
6. `1 kHz`
7. `2 kHz`
8. `4 kHz`
9. `8 kHz`
10. `16 kHz`

Each visible band `MUST` support `-12 dB` to `+12 dB`.

## 6. Required Elements

- Ten vertical sliders
- Clear `+12 dB`, `0 dB`, and `-12 dB` reference labels
- Band labels
- Preset row

## 7. Event Contract

- `EVT_PAGE_ENTER`
  - effect: enter `STATE_EQUALIZER_READY` and ensure one band is selected
- `EVT_NAV_LEFT`
  - effect: select previous band when current band is not the first band
- `EVT_NAV_RIGHT`
  - effect: select next band when current band is not the last band
- `EVT_NAV_UP`
  - effect: increase selected band gain within allowed range
- `EVT_NAV_DOWN`
  - effect: decrease selected band gain within allowed range
- `EVT_BACK`
  - effect: return to previous page

## 8. Transition Contract

- `STATE_EQUALIZER_READY + EVT_NAV_LEFT [guard: selected band is not first band]` -> effect: move selection left -> `STATE_EQUALIZER_READY`
- `STATE_EQUALIZER_READY + EVT_NAV_RIGHT [guard: selected band is not last band]` -> effect: move selection right -> `STATE_EQUALIZER_READY`
- `STATE_EQUALIZER_READY + EVT_NAV_UP [guard: selected band gain is below +12 dB]` -> effect: increase selected band gain -> `STATE_EQUALIZER_READY`
- `STATE_EQUALIZER_READY + EVT_NAV_DOWN [guard: selected band gain is above -12 dB]` -> effect: decrease selected band gain -> `STATE_EQUALIZER_READY`
- `STATE_EQUALIZER_READY + EVT_BACK` -> effect: return to previous page -> previous page

## 8.1 Page-Local F1 Help

- `F1` `MUST` open the `Equalizer` page's own help modal.
- The current `Equalizer` page has no explicit shortcut rows; therefore the modal `MUST` use the empty implementation.
- The modal `MUST NOT` reuse list, playback, or main-menu shortcut content.

## 9. Empty State

- No dedicated empty state exists for `Equalizer`.

## 10. Error State

- No dedicated error page belongs here in the current implementation.
- The page `MUST NOT` fake a full preset-management system if it is not actually implemented.
- The page `MUST NOT` expose fewer than ten bands in the current implementation.
- The implementation `MUST NOT` inherit the old incorrect band-selection mapping as if it were product truth.

## 11. Non-Goals

- Graphical audio analyzer
- Parametric EQ editor
- Separate preamp UI in the current implementation
- `A/B` comparison UI in the current implementation
- `Balance`, `loudness`, `ReplayGain`, or `limiter` controls in the current implementation
- Output-device binding or professional DSP management UI in the current implementation
