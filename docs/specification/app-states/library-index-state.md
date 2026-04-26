<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero App State Contract: LibraryIndexState

## 1. Purpose

- Own the shared runtime truth for library-index readiness, scan/build progress, refresh state, and degraded index health.

## 2. Related Specs

- Shared app-state rules: `docs/specification/lofibox-zero-app-state-spec.md`
- Scan/build domain rules: `docs/specification/lofibox-zero-library-indexing-spec.md`
- Persistence domains: `docs/specification/lofibox-zero-persistence-spec.md`
- Boot projection: `docs/specification/current-ui-pages/boot.md`

## 3. Owned Truth

- whether the library index is unavailable, loading, ready, refreshing, rebuilding, or degraded
- current scan/build mode
- whether browse pages may rely on indexed library truth
- current index progress projection when progress is truthfully available

## 4. Data Dependencies

- configured indexable roots
- persisted library metadata load outcomes
- scan or rebuild outcomes
- file-system discovery and metadata collection results

## 5. State Enumeration

- `STATE_LIBRARY_INDEX_UNINITIALIZED`
  library index work has not started yet
- `STATE_LIBRARY_INDEX_LOADING`
  persisted or freshly built index data is being prepared
- `STATE_LIBRARY_INDEX_READY`
  index is healthy enough for normal browsing
- `STATE_LIBRARY_INDEX_REFRESHING`
  incremental refresh is in progress while prior index truth may still be usable
- `STATE_LIBRARY_INDEX_REBUILDING`
  full rebuild is in progress
- `STATE_LIBRARY_INDEX_DEGRADED`
  index exists or partially exists, but repair, fallback, or partial failure occurred

## 6. Invariants

- Browse pages must not pretend the library is ready before `LibraryIndexState` says so.
- Startup loading projection must be truthful to index readiness.
- Refresh and rebuild are different truths and must not be conflated.
- Degraded index readiness must remain detectable rather than silently reported as fully healthy.

## 7. Event Contract

- `EVT_INDEX_LOAD_STARTED`
  - effect: begin loading or preparing the index
- `EVT_INDEX_LOAD_COMPLETED`
  - effect: mark the index ready for browse use
- `EVT_INDEX_INCREMENTAL_REFRESH_STARTED`
  - effect: begin incremental refresh
- `EVT_INDEX_INCREMENTAL_REFRESH_COMPLETED`
  - effect: complete incremental refresh and return to ready state
- `EVT_INDEX_REBUILD_REQUESTED`
  - effect: begin full rebuild
- `EVT_INDEX_REBUILD_COMPLETED`
  - effect: complete rebuild and return to ready state
- `EVT_INDEX_REPAIR_APPLIED`
  - effect: mark degraded but usable index truth
- `EVT_INDEX_FAILED`
  - effect: mark degraded or unavailable index truth depending on remaining usable data
- `EVT_INDEX_PROGRESS_UPDATED`
  - effect: refresh visible progress projection when real progress is available

## 8. Transition Contract

- `STATE_LIBRARY_INDEX_UNINITIALIZED + EVT_INDEX_LOAD_STARTED` -> effect: begin index preparation -> `STATE_LIBRARY_INDEX_LOADING`
- `STATE_LIBRARY_INDEX_LOADING + EVT_INDEX_PROGRESS_UPDATED` -> effect: refresh progress projection -> `STATE_LIBRARY_INDEX_LOADING`
- `STATE_LIBRARY_INDEX_LOADING + EVT_INDEX_LOAD_COMPLETED` -> effect: mark index ready -> `STATE_LIBRARY_INDEX_READY`
- `STATE_LIBRARY_INDEX_LOADING + EVT_INDEX_REPAIR_APPLIED` -> effect: mark degraded usable index -> `STATE_LIBRARY_INDEX_DEGRADED`
- `STATE_LIBRARY_INDEX_READY + EVT_INDEX_INCREMENTAL_REFRESH_STARTED` -> effect: begin incremental refresh -> `STATE_LIBRARY_INDEX_REFRESHING`
- `STATE_LIBRARY_INDEX_REFRESHING + EVT_INDEX_PROGRESS_UPDATED` -> effect: refresh progress projection -> `STATE_LIBRARY_INDEX_REFRESHING`
- `STATE_LIBRARY_INDEX_REFRESHING + EVT_INDEX_INCREMENTAL_REFRESH_COMPLETED` -> effect: mark refreshed index ready -> `STATE_LIBRARY_INDEX_READY`
- `STATE_LIBRARY_INDEX_READY + EVT_INDEX_REBUILD_REQUESTED` -> effect: begin full rebuild -> `STATE_LIBRARY_INDEX_REBUILDING`
- `STATE_LIBRARY_INDEX_DEGRADED + EVT_INDEX_REBUILD_REQUESTED` -> effect: begin full rebuild -> `STATE_LIBRARY_INDEX_REBUILDING`
- `STATE_LIBRARY_INDEX_REBUILDING + EVT_INDEX_PROGRESS_UPDATED` -> effect: refresh progress projection -> `STATE_LIBRARY_INDEX_REBUILDING`
- `STATE_LIBRARY_INDEX_REBUILDING + EVT_INDEX_REBUILD_COMPLETED` -> effect: mark rebuilt index ready -> `STATE_LIBRARY_INDEX_READY`
- `STATE_LIBRARY_INDEX_LOADING + EVT_INDEX_FAILED` -> effect: mark index unavailable or degraded based on usable data -> `STATE_LIBRARY_INDEX_DEGRADED`
- `STATE_LIBRARY_INDEX_REFRESHING + EVT_INDEX_FAILED` -> effect: preserve prior usable index but mark degraded state -> `STATE_LIBRARY_INDEX_DEGRADED`
- `STATE_LIBRARY_INDEX_REBUILDING + EVT_INDEX_FAILED` -> effect: mark degraded state after rebuild failure -> `STATE_LIBRARY_INDEX_DEGRADED`

## 9. Default State

- Default index state at app start is `STATE_LIBRARY_INDEX_UNINITIALIZED`.

## 10. Error State

- Failed index preparation must not be reported as ready.
- Missing progress data must not be fabricated into fake progress values.
- Partial or degraded index outcomes must remain visible to shared runtime truth.

## 11. Non-Goals

- Search-result state
- Per-page loading spinners beyond the shared library readiness truth
- Fine-grained file-system watcher implementation details
