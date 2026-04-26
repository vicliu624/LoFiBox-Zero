<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero App State Contract: SourceState

## 1. Purpose

- Own the shared source-management truth for local, LAN, URL, station, and media-server source selection.

## 2. Related Specs

- Shared app-state rules: `docs/specification/lofibox-zero-app-state-spec.md`
- Persistence domains: `docs/specification/lofibox-zero-persistence-spec.md`
- Credential rules: `docs/specification/lofibox-zero-credential-spec.md`
- Final product source model: `docs/specification/lofibox-zero-final-product-spec.md`
- Streaming capability boundaries: `docs/specification/lofibox-zero-streaming-spec.md`

## 3. Owned Truth

- saved source list presented to the product
- active source identifier when a current source is selected
- default source identifier when one is configured
- source availability snapshot as projected into shared app state
- current source-management selection context when needed by the UI

## 4. Data Dependencies

- persisted source definitions such as local roots, LAN shares, saved URLs, stations, or media servers
- source capability and availability facts projected from lower-level source or registry components
- source authentication/session readiness projected from `CredentialState` when applicable

## 5. State Enumeration

- `STATE_SOURCE_EMPTY`
  no saved or active sources are available in shared app state
- `STATE_SOURCE_READY`
  one or more saved sources exist and source truth is available

## 6. Invariants

- Saved source management is shared app truth, not a page-local list.
- Persisted source definitions and secure credential references must follow the persistence and credential specs rather than page-local conventions.
- At most one active source identifier is defined at a time.
- A configured default source must refer to a saved source that still exists.
- Availability or capability projections must remain truthful and must not be invented by page logic.
- Source truth must not fork by protocol or server family at the page layer.
- SourceState must not own raw secret material or secret-store availability truth.

## 7. Event Contract

- `EVT_SOURCE_SET_REPLACED`
  - effect: replace the saved source set and recompute active/default projections
- `EVT_SOURCE_ADDED`
  - effect: add one source to the saved source set
- `EVT_SOURCE_UPDATED`
  - effect: update one saved source definition
- `EVT_SOURCE_REMOVED`
  - effect: remove one saved source definition
- `EVT_SOURCE_SELECTED`
  - effect: set the active source identifier
- `EVT_SOURCE_DEFAULT_SET`
  - effect: set the default source identifier
- `EVT_SOURCE_AVAILABILITY_CHANGED`
  - effect: refresh availability or capability projections for affected sources

## 8. Transition Contract

- `STATE_SOURCE_EMPTY + EVT_SOURCE_SET_REPLACED [guard: saved source set becomes non-empty]` -> effect: store sources and resolve default/active projections -> `STATE_SOURCE_READY`
- `STATE_SOURCE_EMPTY + EVT_SOURCE_ADDED` -> effect: create saved source set with the new source -> `STATE_SOURCE_READY`
- `STATE_SOURCE_READY + EVT_SOURCE_ADDED` -> effect: append new source to saved source set -> `STATE_SOURCE_READY`
- `STATE_SOURCE_READY + EVT_SOURCE_UPDATED` -> effect: update matching saved source definition -> `STATE_SOURCE_READY`
- `STATE_SOURCE_READY + EVT_SOURCE_REMOVED [guard: remaining saved sources still exist]` -> effect: remove source and repair active/default projections if needed -> `STATE_SOURCE_READY`
- `STATE_SOURCE_READY + EVT_SOURCE_REMOVED [guard: removed source was the last saved source]` -> effect: clear saved source set and clear active/default projections -> `STATE_SOURCE_EMPTY`
- `STATE_SOURCE_READY + EVT_SOURCE_SELECTED [guard: selected source exists]` -> effect: set active source identifier -> `STATE_SOURCE_READY`
- `STATE_SOURCE_READY + EVT_SOURCE_DEFAULT_SET [guard: default source exists]` -> effect: set default source identifier -> `STATE_SOURCE_READY`
- `STATE_SOURCE_READY + EVT_SOURCE_AVAILABILITY_CHANGED` -> effect: refresh projected availability and capability facts -> `STATE_SOURCE_READY`

## 9. Default State

- Default source state at app start is `STATE_SOURCE_EMPTY` until source definitions are loaded.

## 10. Error State

- Invalid source identifiers `MUST NOT` remain as active or default source truth.
- If a saved source definition becomes unusable, its projected availability must degrade truthfully rather than pretending it is reachable.
- Removing the active or default source must repair source truth rather than leaving dangling identifiers.

## 11. Non-Goals

- Source protocol internals
- Page-specific source-management UI layout
- Streaming transport implementation details
