<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero App State Contract: MetadataServiceState

## 1. Purpose

- Own the shared truth for the built-in metadata-completion service that the current implementation exposes as read-only product behavior.

## 2. Related Specs

- Shared app-state rules: `docs/specification/lofibox-zero-app-state-spec.md`
- Current settings page: `docs/specification/current-ui-pages/settings.md`
- Media pipeline contract: `docs/specification/lofibox-zero-media-pipeline-spec.md`

## 3. Owned Truth

- which metadata-completion service the product currently uses
- whether that service is available in the current implementation
- current read-only display name and status projection

## 4. Data Dependencies

- product-local metadata pipeline configuration
- runtime availability of the current implementation's online metadata-completion path
- current network connectivity projection

## 5. State Enumeration

- `STATE_METADATA_SERVICE_OFFLINE`
  the built-in service exists but online completion is not currently usable because the app is offline
- `STATE_METADATA_SERVICE_READY`
  the built-in service exists and online completion is currently usable

## 6. Invariants

- The current implementation exposes a built-in metadata service, not a user-editable provider selector.
- Online metadata completion must depend on shared network connectivity truth, not on app-managed Wi-Fi configuration semantics.
- `Settings` may project metadata-service truth, but it must not redefine or own it.
- The current implementation must not pretend multiple selectable providers exist when only one built-in provider is present.

## 7. Event Contract

- `EVT_METADATA_SERVICE_LOADED`
  - effect: load built-in metadata-service truth
- `EVT_NETWORK_STATUS_REFRESHED`
  - effect: refresh online-ready projection from shared network state

## 8. Transition Contract

- `STATE_METADATA_SERVICE_OFFLINE + EVT_METADATA_SERVICE_LOADED [guard: network is offline]` -> effect: load built-in service truth -> `STATE_METADATA_SERVICE_OFFLINE`
- `STATE_METADATA_SERVICE_OFFLINE + EVT_METADATA_SERVICE_LOADED [guard: network is connected]` -> effect: load built-in service truth -> `STATE_METADATA_SERVICE_READY`
- `STATE_METADATA_SERVICE_OFFLINE + EVT_NETWORK_STATUS_REFRESHED [guard: network becomes connected]` -> effect: mark online completion ready -> `STATE_METADATA_SERVICE_READY`
- `STATE_METADATA_SERVICE_READY + EVT_NETWORK_STATUS_REFRESHED [guard: network remains connected]` -> effect: keep ready projection -> `STATE_METADATA_SERVICE_READY`
- `STATE_METADATA_SERVICE_READY + EVT_NETWORK_STATUS_REFRESHED [guard: network becomes offline]` -> effect: mark online completion unavailable -> `STATE_METADATA_SERVICE_OFFLINE`

## 9. Default State

- Default state at app start is `STATE_METADATA_SERVICE_OFFLINE` until connectivity truth says otherwise.

## 10. Error State

- Service offline/degraded state must be reflected as read-only status truth rather than turning the settings row into an editable control.

## 11. Non-Goals

- Provider marketplace selection
- User-editable service switching
- Credential ownership
- Network source management
