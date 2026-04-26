<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero App State Contract: SearchState

## 1. Purpose

- Own the shared search truth for the finished product's search surface and unified local-plus-remote search behavior.

## 2. Related Specs

- Shared app-state rules: `docs/specification/lofibox-zero-app-state-spec.md`
- Final product surfaces: `docs/specification/lofibox-zero-final-product-spec.md`
- Streaming search semantics: `docs/specification/lofibox-zero-streaming-spec.md`
- Library browse semantics: `docs/specification/app-states/library-context.md`

## 3. Owned Truth

- active search query
- active search scope
- current result snapshot
- current search lifecycle state
- result grouping semantics when multiple sources contribute

## 4. Data Dependencies

- local catalog search provider
- optional remote or media-server search providers
- user query text
- source-availability and capability information

## 5. State Enumeration

- `STATE_SEARCH_IDLE`
  no active query is being run
- `STATE_SEARCH_QUERY_DIRTY`
  query text exists but results are not current yet
- `STATE_SEARCH_RUNNING`
  a search request is actively resolving
- `STATE_SEARCH_RESULTS_READY`
  one or more results are available
- `STATE_SEARCH_EMPTY`
  query is valid but returned no results

## 6. Invariants

- Search is a shared product surface, not an ad-hoc field owned by one page.
- When local and remote search are merged, results must remain grouped or otherwise explain their source.
- Search truth must not be redefined separately by each source family.
- Empty results are a valid search outcome and must not be faked into non-empty results.

## 7. Event Contract

- `EVT_QUERY_CHANGED`
  - effect: update query text and mark results stale
- `EVT_SEARCH_SUBMITTED`
  - effect: start a search request for the current query and scope
- `EVT_SEARCH_RESULTS_RECEIVED`
  - effect: store results and determine whether they are empty or non-empty
- `EVT_SEARCH_CLEARED`
  - effect: clear query text and results
- `EVT_SEARCH_SCOPE_CHANGED`
  - effect: update active search scope and mark results stale

## 8. Transition Contract

- `STATE_SEARCH_IDLE + EVT_QUERY_CHANGED` -> effect: store query text -> `STATE_SEARCH_QUERY_DIRTY`
- `STATE_SEARCH_QUERY_DIRTY + EVT_QUERY_CHANGED` -> effect: update query text -> `STATE_SEARCH_QUERY_DIRTY`
- `STATE_SEARCH_QUERY_DIRTY + EVT_SEARCH_SCOPE_CHANGED` -> effect: update search scope -> `STATE_SEARCH_QUERY_DIRTY`
- `STATE_SEARCH_QUERY_DIRTY + EVT_SEARCH_SUBMITTED [guard: query is non-empty]` -> effect: start search request -> `STATE_SEARCH_RUNNING`
- `STATE_SEARCH_RUNNING + EVT_SEARCH_RESULTS_RECEIVED [guard: result set is non-empty]` -> effect: store grouped results -> `STATE_SEARCH_RESULTS_READY`
- `STATE_SEARCH_RUNNING + EVT_SEARCH_RESULTS_RECEIVED [guard: result set is empty]` -> effect: store empty result snapshot -> `STATE_SEARCH_EMPTY`
- `STATE_SEARCH_RESULTS_READY + EVT_QUERY_CHANGED` -> effect: update query text and invalidate prior results -> `STATE_SEARCH_QUERY_DIRTY`
- `STATE_SEARCH_RESULTS_READY + EVT_SEARCH_SCOPE_CHANGED` -> effect: change scope and invalidate prior results -> `STATE_SEARCH_QUERY_DIRTY`
- `STATE_SEARCH_EMPTY + EVT_QUERY_CHANGED` -> effect: update query text and invalidate prior results -> `STATE_SEARCH_QUERY_DIRTY`
- `STATE_SEARCH_EMPTY + EVT_SEARCH_SCOPE_CHANGED` -> effect: change scope and invalidate prior results -> `STATE_SEARCH_QUERY_DIRTY`
- `STATE_SEARCH_IDLE + EVT_SEARCH_CLEARED` -> effect: keep query and results empty -> `STATE_SEARCH_IDLE`
- `STATE_SEARCH_QUERY_DIRTY + EVT_SEARCH_CLEARED` -> effect: clear query and results -> `STATE_SEARCH_IDLE`
- `STATE_SEARCH_RUNNING + EVT_SEARCH_CLEARED` -> effect: clear query and discard in-flight result ownership -> `STATE_SEARCH_IDLE`
- `STATE_SEARCH_RESULTS_READY + EVT_SEARCH_CLEARED` -> effect: clear query and results -> `STATE_SEARCH_IDLE`
- `STATE_SEARCH_EMPTY + EVT_SEARCH_CLEARED` -> effect: clear query and results -> `STATE_SEARCH_IDLE`

## 9. Default State

- Default search state is `STATE_SEARCH_IDLE`.

## 10. Error State

- Invalid provider failures `MUST NOT` fabricate search results.
- Provider-specific failures should degrade to no results or recoverable search state rather than corrupt query truth.

## 11. Non-Goals

- Current-page search UI in the current implementation
- Search-result ranking internals
- Persisted search history
