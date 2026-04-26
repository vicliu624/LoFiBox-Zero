<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero App State Contract: NetworkState

## 1. Purpose

- Own the shared truth for current connectivity status projection.

## 2. Related Specs

- Shared app-state rules: `docs/specification/lofibox-zero-app-state-spec.md`
- Current settings page: `docs/specification/current-ui-pages/settings.md`
- Credential handling rules: `docs/specification/lofibox-zero-credential-spec.md`

## 3. Owned Truth

- current network connectivity status projection
- whether the current implementation is projecting host/container networking or device-side networking
- current read-only network status label

## 4. Data Dependencies

- current runtime environment
- current host or device network-readiness probe
- current runtime connectivity probe

## 5. State Enumeration

- `STATE_NETWORK_OFFLINE`
  the current implementation reports no usable network connectivity
- `STATE_NETWORK_READY`
  the current implementation reports a truthful network-ready result

## 6. Invariants

- NetworkState owns connectivity truth; `SettingsState` must not absorb it.
- Host/container mode must remain explicit when the runtime is not actually managing a real Wi-Fi radio.
- The app must not pretend it manages system Wi-Fi configuration when that responsibility belongs to the host system.

## 7. Event Contract

- `EVT_NETWORK_STATUS_REFRESHED`
  - effect: refresh truthful network projection

## 8. Transition Contract

- `STATE_NETWORK_OFFLINE + EVT_NETWORK_STATUS_REFRESHED [guard: connectivity probe succeeds]` -> effect: report ready -> `STATE_NETWORK_READY`
- `STATE_NETWORK_OFFLINE + EVT_NETWORK_STATUS_REFRESHED [guard: connectivity probe still fails]` -> effect: keep offline projection -> `STATE_NETWORK_OFFLINE`
- `STATE_NETWORK_READY + EVT_NETWORK_STATUS_REFRESHED [guard: connectivity probe succeeds]` -> effect: keep ready projection -> `STATE_NETWORK_READY`
- `STATE_NETWORK_READY + EVT_NETWORK_STATUS_REFRESHED [guard: connectivity probe fails]` -> effect: degrade to offline projection -> `STATE_NETWORK_OFFLINE`

## 9. Default State

- Default state at app start is `STATE_NETWORK_OFFLINE`.

## 10. Error State

- Negative readiness results must surface as explicit offline truth rather than pretending success.
- Host/container mode must not be mislabeled as direct radio control.

## 11. Non-Goals

- Full Wi-Fi credential management
- Account/session authentication
- Media-server source ownership
- Transport-layer streaming playback behavior
