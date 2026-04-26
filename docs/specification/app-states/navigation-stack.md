<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero App State Contract: NavigationStack

## 1. Purpose

- Own cross-page navigation history and `Back` behavior.

## 2. Related Specs

- Shared app-state rules: `docs/specification/lofibox-zero-app-state-spec.md`
- Current UI rules: `docs/specification/lofibox-zero-page-spec.md`
- Current page files: `docs/specification/current-ui-pages/`

## 3. Owned Truth

- current page identity
- back-stack history
- root-page identity after startup handoff
- page entry payload carried with pushed pages when needed

## 4. Data Dependencies

- page identity tokens
- optional page parameters or context payloads
- startup handoff signal from `Boot`

## 5. State Enumeration

- `STATE_NAVIGATION_PRE_ROOT`
  startup has not yet established the normal app root
- `STATE_NAVIGATION_ROOT_ONLY`
  stack contains only the root page
- `STATE_NAVIGATION_WITH_HISTORY`
  stack contains the root page plus one or more deeper pages

## 6. Invariants

- After startup handoff, `Main Menu` is the root page in the current implementation.
- The active page is always the top of the stack.
- `Back` returns to the previous page in the stack whenever history exists.
- Root-state `Back` must not pop the root away.
- `Boot` is a startup surface and does not participate in ordinary post-startup back-stack behavior.

## 7. Event Contract

- `EVT_BOOT_COMPLETED`
  - effect: establish root page as `Main Menu`
- `EVT_PUSH_PAGE`
  - effect: push a new page and make it active
- `EVT_BACK`
  - effect: pop one page when history exists
- `EVT_POP_TO_ROOT`
  - effect: clear history and make root page active

## 8. Transition Contract

- `STATE_NAVIGATION_PRE_ROOT + EVT_BOOT_COMPLETED` -> effect: create stack with `Main Menu` as root -> `STATE_NAVIGATION_ROOT_ONLY`
- `STATE_NAVIGATION_ROOT_ONLY + EVT_PUSH_PAGE` -> effect: push destination page above root -> `STATE_NAVIGATION_WITH_HISTORY`
- `STATE_NAVIGATION_ROOT_ONLY + EVT_BACK` -> effect: remain at root page -> `STATE_NAVIGATION_ROOT_ONLY`
- `STATE_NAVIGATION_WITH_HISTORY + EVT_PUSH_PAGE` -> effect: push destination page above current top -> `STATE_NAVIGATION_WITH_HISTORY`
- `STATE_NAVIGATION_WITH_HISTORY + EVT_BACK [guard: stack depth becomes 1 after pop]` -> effect: pop active page and expose root -> `STATE_NAVIGATION_ROOT_ONLY`
- `STATE_NAVIGATION_WITH_HISTORY + EVT_BACK [guard: stack depth remains above 1 after pop]` -> effect: pop active page and expose previous page -> `STATE_NAVIGATION_WITH_HISTORY`
- `STATE_NAVIGATION_WITH_HISTORY + EVT_POP_TO_ROOT` -> effect: clear history above root -> `STATE_NAVIGATION_ROOT_ONLY`

## 9. Default State

- Default state at process start is `STATE_NAVIGATION_PRE_ROOT`.
- Default steady-state root after startup is `STATE_NAVIGATION_ROOT_ONLY` with `Main Menu` active.

## 10. Error State

- Popping from an empty or root-only stack `MUST NOT` corrupt navigation state.
- Invalid push targets `MUST NOT` create ghost pages in the stack.

## 11. Non-Goals

- Browser-style multi-branch history
- Multiple independent modal stacks in the current implementation
- Deep-link restoration rules
