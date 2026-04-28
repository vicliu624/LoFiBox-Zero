<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Current UI Page Spec: Search

## 1. Purpose

- Provide the current small-screen search surface for local and remote playable media.
- Let the user type a Unicode query, inspect grouped source-aware results, and start playback without learning provider internals.

This page projects `SearchState`.
It does not own input-method engines, remote provider protocol details, or final search ranking truth.

## 2. Related Specs

- Shared current UI rules: `docs/specification/lofibox-zero-page-spec.md`
- Shared search state: `docs/specification/app-states/search-state.md`
- Text input and input method boundary: `docs/specification/lofibox-zero-text-input-spec.md`
- Streaming search semantics: `docs/specification/lofibox-zero-streaming-spec.md`
- Library indexing and search governance: `docs/specification/lofibox-zero-library-indexing-spec.md`
- Layout template: `docs/specification/lofibox-zero-layout-spec.md`
- Visual styling: `docs/specification/lofibox-zero-visual-design-spec.md`

## 3. Entry Conditions

- `Search` is entered by global `F9` from any non-boot page.
- Future page-local search affordances may enter this page if they preserve the same `SearchState` truth.

## 4. Data Dependencies

- `SearchState` query, scope, lifecycle, and result snapshot
- committed UTF-8 text events from the active runtime shell
- optional transient preedit text from the active runtime shell
- local library catalog search provider
- configured remote providers that expose search capability
- normalized `MediaItem` values for local and remote results
- current selected row

## 5. State Enumeration

- `STATE_SEARCH_IDLE`
  no committed query is present
- `STATE_SEARCH_EDITING`
  committed query exists and results are stale or being refreshed
- `STATE_SEARCH_RESULTS_READY`
  one or more grouped results are visible
- `STATE_SEARCH_EMPTY`
  query is valid but no result is available
- `STATE_SEARCH_DEGRADED`
  at least one source failed or timed out while other search truth remains valid

## 6. Required Elements

- standard top bar with title `SEARCH`
- query row showing committed query text or `TYPE...`
- optional preedit presentation when a system input method is composing text
- source/scope status row when it helps explain what is searched
- source-aware result rows
- result source labels in the secondary column or explicit group headers
- stable empty row when no results exist
- page-local `F1` help

The page may reuse the compact list shell in the current implementation.
It must still remain a first-class page with its own title, help rows, state, and empty/error behavior.

## 7. Event Contract

- `EVT_PAGE_ENTER`
  - effect: enter `STATE_SEARCH_IDLE` or project the current `SearchState`
- `EVT_TEXT_COMMITTED`
  - effect: append committed UTF-8 text to query and refresh or mark results dirty
- `EVT_PREEDIT_CHANGED`
  - effect: update transient composition display without mutating committed query
- `EVT_TEXT_BACKSPACE`
  - effect: delete one Unicode-safe text unit from committed query
- `EVT_CLEAR_QUERY`
  - effect: clear committed query, preedit projection, and result snapshot
- `EVT_NAV_UP`
  - effect: move selected actionable row up with list wrap behavior
- `EVT_NAV_DOWN`
  - effect: move selected actionable row down with list wrap behavior
- `EVT_PAGE_UP`
  - effect: move selection by one viewport
- `EVT_PAGE_DOWN`
  - effect: move selection by one viewport
- `EVT_CONFIRM`
  - effect: start playback for the selected playable result
- `EVT_SEARCH_RESULTS_DEGRADED`
  - effect: show truthful partial-source failure state while preserving valid query and usable result groups
- `EVT_BACK`
  - effect: return to previous page
- `EVT_HELP`
  - effect: open Search page help modal

## 8. Transition Contract

- `STATE_SEARCH_IDLE + EVT_TEXT_COMMITTED [guard: text is valid UTF-8]` -> effect: store query and search -> `STATE_SEARCH_EDITING`
- `STATE_SEARCH_EDITING + EVT_TEXT_COMMITTED [guard: text is valid UTF-8]` -> effect: update query and refresh results -> `STATE_SEARCH_EDITING`
- `STATE_SEARCH_RESULTS_READY + EVT_TEXT_COMMITTED [guard: text is valid UTF-8]` -> effect: update query and invalidate prior results -> `STATE_SEARCH_EDITING`
- `STATE_SEARCH_EMPTY + EVT_TEXT_COMMITTED [guard: text is valid UTF-8]` -> effect: update query and retry search -> `STATE_SEARCH_EDITING`
- `STATE_SEARCH_EDITING + EVT_TEXT_BACKSPACE [guard: query remains non-empty]` -> effect: update query and refresh results -> `STATE_SEARCH_EDITING`
- `STATE_SEARCH_EDITING + EVT_TEXT_BACKSPACE [guard: query becomes empty]` -> effect: clear results -> `STATE_SEARCH_IDLE`
- `STATE_SEARCH_RESULTS_READY + EVT_TEXT_BACKSPACE [guard: query remains non-empty]` -> effect: update query and invalidate prior results -> `STATE_SEARCH_EDITING`
- `STATE_SEARCH_RESULTS_READY + EVT_TEXT_BACKSPACE [guard: query becomes empty]` -> effect: clear results -> `STATE_SEARCH_IDLE`
- `STATE_SEARCH_EDITING + EVT_PREEDIT_CHANGED` -> effect: update preedit projection only -> `STATE_SEARCH_EDITING`
- `STATE_SEARCH_RESULTS_READY + EVT_PREEDIT_CHANGED` -> effect: update preedit projection only -> `STATE_SEARCH_RESULTS_READY`
- `STATE_SEARCH_EDITING + EVT_SEARCH_RESULTS_RECEIVED [guard: results exist]` -> effect: store grouped result snapshot -> `STATE_SEARCH_RESULTS_READY`
- `STATE_SEARCH_EDITING + EVT_SEARCH_RESULTS_RECEIVED [guard: no results]` -> effect: store empty result snapshot -> `STATE_SEARCH_EMPTY`
- `STATE_SEARCH_EDITING + EVT_SEARCH_RESULTS_DEGRADED [guard: at least one source still returns usable results]` -> effect: store results with source degradation status -> `STATE_SEARCH_DEGRADED`
- `STATE_SEARCH_RESULTS_READY + EVT_CONFIRM [guard: selected row is playable]` -> effect: start result playback -> `Now Playing`
- `STATE_SEARCH_DEGRADED + EVT_CONFIRM [guard: selected row is playable]` -> effect: start result playback -> `Now Playing`
- `STATE_SEARCH_DEGRADED + EVT_TEXT_COMMITTED [guard: text is valid UTF-8]` -> effect: update query and retry search -> `STATE_SEARCH_EDITING`
- `STATE_SEARCH_DEGRADED + EVT_TEXT_BACKSPACE [guard: query remains non-empty]` -> effect: update query and retry search -> `STATE_SEARCH_EDITING`
- `STATE_SEARCH_DEGRADED + EVT_TEXT_BACKSPACE [guard: query becomes empty]` -> effect: clear results -> `STATE_SEARCH_IDLE`
- `STATE_SEARCH_EMPTY + EVT_TEXT_BACKSPACE [guard: query remains non-empty]` -> effect: update query and retry search -> `STATE_SEARCH_EDITING`
- `STATE_SEARCH_EMPTY + EVT_TEXT_BACKSPACE [guard: query becomes empty]` -> effect: clear results -> `STATE_SEARCH_IDLE`
- `STATE_SEARCH_RESULTS_READY + EVT_CLEAR_QUERY` -> effect: clear query and results -> `STATE_SEARCH_IDLE`
- `STATE_SEARCH_DEGRADED + EVT_CLEAR_QUERY` -> effect: clear query and results -> `STATE_SEARCH_IDLE`
- `STATE_SEARCH_EMPTY + EVT_CLEAR_QUERY` -> effect: clear query and results -> `STATE_SEARCH_IDLE`
- `STATE_SEARCH_IDLE + EVT_BACK` -> effect: return to previous page -> previous page
- `STATE_SEARCH_RESULTS_READY + EVT_BACK` -> effect: return to previous page -> previous page

## 8.1 Page-Local F1 Help

- `F1` `MUST` open the `Search` page's own help modal.
- The Search help modal `MUST` include:
  - `TYPE`: query
  - `BACKSPACE`: edit
  - `DEL`: clear query when implemented
  - `UP/DOWN`: move result selection
  - `PGUP/PGDN`: page result selection
  - `OK`: play selected result
  - global playback shortcuts
  - `HOME`: return to Main Menu

## 8.2 Text Input And IME Rules

- The committed query `MUST` be valid UTF-8.
- CJK input must enter the page as committed UTF-8 text from the runtime shell's system input-method path.
- Preedit text `MUST NOT` mutate committed query or trigger committed-search results.
- `BACKSPACE` must delete by Unicode-safe text unit, not by raw byte.
- The page `MUST NOT` implement Pinyin, Kana, Hangul, Rime, Mozc, or other general input-method engines.
- If the active runtime shell cannot provide CJK committed text, the page must not claim that CJK IME is supported on that shell.

## 9. Empty State

- With no query, the page should show a stable prompt such as `TYPE...`.
- With a query and no results, the page must show `NO RESULTS` or equivalent.
- Empty state rows must allow continued text input, help, and back navigation.

## 10. Error State

- A local search failure must not fabricate results.
- A remote source timeout or provider failure must degrade that source's result group without corrupting query truth.
- If some sources succeed and others fail, visible results may remain available but their source explanation must stay truthful.
- Invalid UTF-8 input must be rejected or repaired before it can enter committed query state.

## 11. Non-Goals

- General-purpose CJK input method implementation
- Search history persistence
- Full-text ranking internals
- Provider-specific protocol fields in UI rows
- Editing metadata from search results
- Exposing raw remote stream URLs from the Search page
