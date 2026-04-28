<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Current UI Page Spec: Remote Profile Settings

## 1. Purpose

- Configure one saved remote source profile after the user has selected a supported source kind.
- Keep source-kind choice, durable profile settings, credentials, TLS policy, permissions, and connection diagnostics separated from the generic Settings page.

This page projects `SourceState`, `CredentialState`, and persisted source-profile truth.
It does not own provider protocol parsing, raw secret storage, or playback behavior.

## 2. Related Specs

- Shared current UI rules: `docs/specification/lofibox-zero-page-spec.md`
- Streaming source rules: `docs/specification/lofibox-zero-streaming-spec.md`
- Persistence rules: `docs/specification/lofibox-zero-persistence-spec.md`
- Credential rules: `docs/specification/lofibox-zero-credential-spec.md`
- Text input and input method boundary: `docs/specification/lofibox-zero-text-input-spec.md`
- Layout template: `docs/specification/lofibox-zero-layout-spec.md`

## 3. Entry Conditions

- `Remote Profile Settings` is entered from `Remote Setup` after a source kind is selected.
- If no profile of that kind exists, the page may receive a new unsaved draft profile for that kind.
- If a profile already exists, the page projects that profile's current durable fields and runtime status.

## 4. Data Dependencies

- selected remote source kind
- provider manifest display name and capability flags
- persisted source profile draft or existing profile
- credential reference and secure credential readiness
- runtime connection/probe status
- TLS/certificate policy state
- read-only or writable source capability state
- current selected row

## 5. State Enumeration

- `STATE_REMOTE_PROFILE_READY`
  profile rows are visible and one row is selected
- `STATE_REMOTE_PROFILE_TESTING`
  a connection test is running
- `STATE_REMOTE_PROFILE_DIRTY`
  one or more profile fields have unsaved changes
- `STATE_REMOTE_PROFILE_DEGRADED`
  profile is visible but credential, connectivity, TLS, or provider capability state is incomplete or failed

## 6. Required Elements

- standard top bar with the selected source kind or profile name
- source kind display row
- address, base URL, share target, station URL, or service root row when the selected kind needs it
- username, account, token, or anonymous/auth mode row according to provider manifest capability
- credential row that is redacted, `MISSING`, `OPTIONAL`, or `N/A` according to credential rules
- TLS or certificate policy row when the selected kind uses network TLS
- permission or source-capability row when read-only versus writable behavior matters
- connection test row
- save/apply row when profile changes are dirty
- stable degraded or missing-setup status when the profile cannot be used yet
- page-local `F1` help

The page must list only fields meaningful for the selected source kind.
It must not force credential rows onto credential-free sources or make optional-auth sources look broken merely because a credential is empty.

## 7. Event Contract

- `EVT_PAGE_ENTER`
  - effect: enter `STATE_REMOTE_PROFILE_READY` and select the first actionable row
- `EVT_NAV_UP`
  - effect: move selection up with list wrap behavior
- `EVT_NAV_DOWN`
  - effect: move selection down with list wrap behavior
- `EVT_CONFIRM`
  - effect: edit the selected editable field, run test, save changes, or activate the row's truthful current behavior
- `EVT_PROFILE_FIELD_CHANGED`
  - effect: update the draft profile and mark it dirty
- `EVT_TEST_CONNECTION`
  - effect: run the selected source kind's probe without exposing raw secrets
- `EVT_TEST_SUCCEEDED`
  - effect: update runtime availability and clear connection-test failure status
- `EVT_TEST_FAILED`
  - effect: update runtime availability and show source-level failure explanation
- `EVT_SAVE_PROFILE`
  - effect: persist durable profile fields and credential references according to persistence and credential rules
- `EVT_BACK`
  - effect: return to `Remote Setup`
- `EVT_HELP`
  - effect: open Remote Profile Settings page help modal

## 8. Transition Contract

- `STATE_REMOTE_PROFILE_READY + EVT_CONFIRM [guard: selected row is editable field]` -> effect: open field editor for the selected field -> `Remote Field Editor`
- `STATE_REMOTE_PROFILE_READY + EVT_CONFIRM [guard: selected row = Test Connection]` -> effect: start probe -> `STATE_REMOTE_PROFILE_TESTING`
- `STATE_REMOTE_PROFILE_DIRTY + EVT_CONFIRM [guard: selected row = Save]` -> effect: persist draft profile -> `STATE_REMOTE_PROFILE_READY`
- `STATE_REMOTE_PROFILE_TESTING + EVT_TEST_SUCCEEDED` -> effect: update runtime status -> `STATE_REMOTE_PROFILE_READY`
- `STATE_REMOTE_PROFILE_TESTING + EVT_TEST_FAILED` -> effect: update degraded status -> `STATE_REMOTE_PROFILE_DEGRADED`
- `STATE_REMOTE_PROFILE_DEGRADED + EVT_PROFILE_FIELD_CHANGED` -> effect: update draft and keep degraded explanation visible -> `STATE_REMOTE_PROFILE_DIRTY`
- `STATE_REMOTE_PROFILE_READY + EVT_BACK` -> effect: return to remote setup -> `Remote Setup`
- `STATE_REMOTE_PROFILE_DIRTY + EVT_BACK` -> effect: keep unsaved changes policy explicit and return or remain according to implementation decision -> `Remote Setup` or `STATE_REMOTE_PROFILE_DIRTY`

## 8.1 Page-Local F1 Help

- `F1` `MUST` open this page's own help modal.
- The modal `MUST` include:
  - `UP/DOWN`: move field selection
  - `OK`: edit, test, or save selected row
  - `BACKSPACE`: back
  - `F9`: search
  - global playback shortcuts

## 9. Empty State

- This page should not be entered without a selected source kind.
- If entered without a valid kind, show a stable `NO PROFILE` degraded row and allow back navigation.

## 10. Error State

- Raw passwords, tokens, cookies, or auth headers must never be displayed.
- A missing provider implementation must appear as unavailable capability or failed test status, not as a corrupted profile.
- TLS or certificate failures must be shown as connection-risk status, not silently ignored.
- Persist failures must remain visible until the user retries, edits, or leaves the page.

## 11. Non-Goals

- Listing source kinds; that belongs to `Remote Setup`
- Browsing remote media catalogs
- Playing test streams directly from this page
- Exposing raw stream URLs or credential material
- Implementing provider-specific protocol forms inside shared page code
