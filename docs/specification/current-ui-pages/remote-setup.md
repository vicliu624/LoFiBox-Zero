<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Current UI Page Spec: Remote Setup

## 1. Purpose

- Let the user choose which supported remote media source kind they want to configure.

## 2. Related Specs

- Shared current UI rules: `docs/specification/lofibox-zero-page-spec.md`
- Streaming source rules: `docs/specification/lofibox-zero-streaming-spec.md`
- Layout template: `docs/specification/lofibox-zero-layout-spec.md`

## 3. Entry Conditions

- `Remote Setup` is entered from the `Remote Setup` row on `Settings`.

## 4. Data Dependencies

- `RemoteSourceRegistry` provider manifests
- current selected row

## 5. State Enumeration

- `STATE_REMOTE_SETUP_READY`
  supported remote source kinds are listed and one row is selected

The page `MUST` list every supported remote source kind before showing profile fields.
Profile fields such as address, username, password, token, TLS, permissions, and test status belong on the next page after a kind is selected.

## 6. Required Elements

- page title
- one row per supported remote source kind
- row label from the provider manifest display name
- compact capability or permission summary in the secondary column

## 7. Event Contract

- `EVT_PAGE_ENTER`
  - effect: enter `STATE_REMOTE_SETUP_READY` and select the first source kind
- `EVT_NAV_UP`
  - effect: move selection up with list wrap behavior
- `EVT_NAV_DOWN`
  - effect: move selection down with list wrap behavior
- `EVT_CONFIRM`
  - effect: open or create a profile settings page for the selected source kind
- `EVT_BACK`
  - effect: return to `Settings`

## 8. Transition Contract

- `STATE_REMOTE_SETUP_READY + EVT_CONFIRM [guard: selected kind has an existing profile]` -> effect: open that profile's settings -> `Remote Profile Settings`
- `STATE_REMOTE_SETUP_READY + EVT_CONFIRM [guard: selected kind has no existing profile]` -> effect: create a new profile of that kind and open it -> `Remote Profile Settings`
- `STATE_REMOTE_SETUP_READY + EVT_BACK` -> effect: return to settings -> `Settings`

## 8.1 Page-Local F1 Help

- `F1` `MUST` open this page's own help modal.

## 9. Empty State

- Empty state should only appear if no remote source kinds are compiled into the registry.
- It must show a stable `NO ITEMS` style row and allow back navigation.

## 10. Error State

- Missing provider implementation must be represented as an unavailable capability or failed test on the profile page, not as a missing source kind here.

## 11. Non-Goals

- Editing address, account, credential, TLS, or permission fields directly on this page
- Showing raw provider protocol names instead of user-facing source kinds
- Exposing raw stream URLs or credentials
