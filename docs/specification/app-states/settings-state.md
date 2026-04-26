<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero App State Contract: SettingsState

## 1. Purpose

- Own the shared truth for persisted player and device preference values that the current implementation exposes through `Settings`.

## 2. Related Specs

- Shared app-state rules: `docs/specification/lofibox-zero-app-state-spec.md`
- Persistence domains: `docs/specification/lofibox-zero-persistence-spec.md`
- Current settings page: `docs/specification/current-ui-pages/settings.md`
- Final product identity: `docs/specification/lofibox-zero-final-product-spec.md`

## 3. Owned Truth

- current sleep-timer setting
- current backlight policy
- current language setting
- loaded-versus-unloaded status of persisted settings values

SettingsState does not own playback shuffle truth; the `Shuffle Songs` row projects playback-session truth.

## 4. Data Dependencies

- persisted settings store
- current device capability information for supported values
- current product-local defaults

## 5. State Enumeration

- `STATE_SETTINGS_UNINITIALIZED`
  settings have not yet been loaded into shared app state
- `STATE_SETTINGS_READY`
  settings values are loaded and available for projection into the UI

## 6. Invariants

- Settings values shown in the UI must be truthful projections of shared settings state or device capability.
- Unsupported device-dependent settings must degrade gracefully rather than pretending support.
- SettingsState must not duplicate playback-session ownership of shuffle or repeat behavior.

## 7. Event Contract

- `EVT_SETTINGS_LOADED`
  - effect: load persisted or default settings values into shared state
- `EVT_SLEEP_TIMER_CHANGED`
  - effect: update sleep-timer value
- `EVT_BACKLIGHT_CHANGED`
  - effect: update backlight policy
- `EVT_LANGUAGE_CHANGED`
  - effect: update current language selection
- `EVT_DEVICE_CAPABILITIES_CHANGED`
  - effect: refresh capability-aware settings projections

## 8. Transition Contract

- `STATE_SETTINGS_UNINITIALIZED + EVT_SETTINGS_LOADED` -> effect: load settings values -> `STATE_SETTINGS_READY`
- `STATE_SETTINGS_READY + EVT_SLEEP_TIMER_CHANGED` -> effect: update sleep timer -> `STATE_SETTINGS_READY`
- `STATE_SETTINGS_READY + EVT_BACKLIGHT_CHANGED` -> effect: update backlight policy -> `STATE_SETTINGS_READY`
- `STATE_SETTINGS_READY + EVT_LANGUAGE_CHANGED` -> effect: update language selection -> `STATE_SETTINGS_READY`
- `STATE_SETTINGS_READY + EVT_DEVICE_CAPABILITIES_CHANGED` -> effect: refresh capability-aware projections -> `STATE_SETTINGS_READY`

## 9. Default State

- Default state at app start is `STATE_SETTINGS_UNINITIALIZED`.
- After settings load, steady state is `STATE_SETTINGS_READY`.

## 10. Error State

- Invalid persisted settings values `MUST` degrade to safe defaults rather than leaving settings truth undefined.
- Unsupported values `MUST NOT` appear as active valid choices in the UI when the device cannot actually honor them.

## 11. Non-Goals

- Network or account settings
- Developer or debug settings
- Full device-administration state
