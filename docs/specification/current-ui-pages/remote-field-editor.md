<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Current UI Page Spec: Remote Field Editor

## 1. Purpose

- Edit one remote source profile field on the small-screen UI.
- Provide a reusable text-entry surface for source address, username, display name, token reference, TLS note, or other source-kind-specific profile fields.

This page projects the selected profile field.
It does not own system input-method engines, raw secret persistence, provider protocol parsing, or connection probing.

## 2. Related Specs

- Shared current UI rules: `docs/specification/lofibox-zero-page-spec.md`
- Text input and input method boundary: `docs/specification/lofibox-zero-text-input-spec.md`
- Streaming source rules: `docs/specification/lofibox-zero-streaming-spec.md`
- Persistence rules: `docs/specification/lofibox-zero-persistence-spec.md`
- Credential rules: `docs/specification/lofibox-zero-credential-spec.md`
- Layout template: `docs/specification/lofibox-zero-layout-spec.md`

## 3. Entry Conditions

- `Remote Field Editor` is entered from `Remote Profile Settings` by confirming an editable field.
- The selected field must declare whether its value is plain text, URL-like text, username-like text, secret material, or an enumerated choice.

## 4. Data Dependencies

- selected source profile draft
- selected field descriptor
- current committed field text or redacted secret state
- optional transient preedit text from the active runtime shell
- credential reference target when the field edits secret material
- validation status for the field

## 5. State Enumeration

- `STATE_REMOTE_FIELD_EDITING`
  committed field text is editable
- `STATE_REMOTE_FIELD_PREEDIT`
  an input method is composing transient preedit text
- `STATE_REMOTE_FIELD_INVALID`
  current committed text fails field validation
- `STATE_REMOTE_FIELD_SECRET`
  the field represents secret material and visible text is redacted

## 6. Required Elements

- standard top bar with field title or source kind
- field label
- committed value row or redacted secret placeholder
- optional preedit row when composition is active
- validation/status row
- save/accept affordance
- cancel/back behavior
- page-local `F1` help

Secret fields must be visibly redacted.
Saving a secret field stores or updates secure credential material and returns a `credential_ref`; it must not write the raw secret into ordinary source-profile records.

## 7. Event Contract

- `EVT_PAGE_ENTER`
  - effect: enter editing state with the field's current committed value
- `EVT_TEXT_COMMITTED`
  - effect: append committed UTF-8 text to the field buffer
- `EVT_PREEDIT_CHANGED`
  - effect: update transient preedit projection without mutating committed field text
- `EVT_TEXT_BACKSPACE`
  - effect: delete one Unicode-safe text unit from the committed field buffer
- `EVT_CLEAR_FIELD`
  - effect: clear the committed field buffer when the field allows clearing
- `EVT_CONFIRM`
  - effect: validate and apply the edited value
- `EVT_BACK`
  - effect: cancel editing and return to `Remote Profile Settings`
- `EVT_HELP`
  - effect: open Remote Field Editor help modal

## 8. Transition Contract

- `EVT_PAGE_ENTER [guard: field is secret material]` -> effect: enter redacted secret editing mode -> `STATE_REMOTE_FIELD_SECRET`
- `EVT_PAGE_ENTER [guard: field is ordinary editable text]` -> effect: enter editing mode with current value -> `STATE_REMOTE_FIELD_EDITING`
- `STATE_REMOTE_FIELD_EDITING + EVT_TEXT_COMMITTED [guard: text is valid UTF-8]` -> effect: append text and validate -> `STATE_REMOTE_FIELD_EDITING` or `STATE_REMOTE_FIELD_INVALID`
- `STATE_REMOTE_FIELD_SECRET + EVT_TEXT_COMMITTED [guard: text is valid UTF-8]` -> effect: update secret buffer without exposing raw value -> `STATE_REMOTE_FIELD_SECRET`
- `STATE_REMOTE_FIELD_SECRET + EVT_TEXT_BACKSPACE` -> effect: delete one Unicode-safe text unit from secret buffer without exposing raw value -> `STATE_REMOTE_FIELD_SECRET`
- `STATE_REMOTE_FIELD_EDITING + EVT_PREEDIT_CHANGED` -> effect: update preedit projection only -> `STATE_REMOTE_FIELD_PREEDIT`
- `STATE_REMOTE_FIELD_PREEDIT + EVT_TEXT_COMMITTED [guard: text is valid UTF-8]` -> effect: append committed text and clear preedit projection -> `STATE_REMOTE_FIELD_EDITING`
- `STATE_REMOTE_FIELD_EDITING + EVT_TEXT_BACKSPACE` -> effect: remove one Unicode-safe text unit and validate -> `STATE_REMOTE_FIELD_EDITING` or `STATE_REMOTE_FIELD_INVALID`
- `STATE_REMOTE_FIELD_INVALID + EVT_TEXT_COMMITTED [guard: text is valid UTF-8]` -> effect: append text and revalidate -> `STATE_REMOTE_FIELD_EDITING` or `STATE_REMOTE_FIELD_INVALID`
- `STATE_REMOTE_FIELD_EDITING + EVT_CONFIRM [guard: field validates]` -> effect: apply field to draft profile -> `Remote Profile Settings`
- `STATE_REMOTE_FIELD_SECRET + EVT_CONFIRM [guard: secret material accepted]` -> effect: store secret through credential store and apply credential reference -> `Remote Profile Settings`
- `STATE_REMOTE_FIELD_EDITING + EVT_BACK` -> effect: cancel edit -> `Remote Profile Settings`
- `STATE_REMOTE_FIELD_INVALID + EVT_BACK` -> effect: cancel edit -> `Remote Profile Settings`

## 8.1 Page-Local F1 Help

- `F1` `MUST` open this page's own help modal.
- The modal `MUST` include:
  - `TYPE`: edit field
  - `BACKSPACE`: edit
  - `DEL`: clear field when implemented
  - `OK`: save field
  - `BACKSPACE` after modal closes: cancel/back
  - global playback shortcuts

## 8.2 Text Input And IME Rules

- Committed text `MUST` be valid UTF-8.
- Preedit text `MUST NOT` mutate committed field text.
- `BACKSPACE` must delete by Unicode-safe text unit, not by raw byte.
- The page `MUST NOT` implement a general CJK input method.
- The active runtime shell's text-input capability must be represented truthfully.

## 9. Empty State

- If no editable field is provided, show a stable `NO FIELD` row and allow back navigation.

## 10. Error State

- Invalid URLs, empty required fields, invalid auth modes, or unsupported TLS policy values must be shown as field validation status.
- Invalid UTF-8 input must be rejected or repaired before it can enter committed field state.
- Secret storage failures must not be presented as profile-save success.

## 11. Non-Goals

- Source-kind selection
- Connection testing
- Remote catalog browsing
- Search
- Raw credential display
- Provider-specific protocol forms in shared page code
