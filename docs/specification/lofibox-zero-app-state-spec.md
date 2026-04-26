<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Application State Specification

## 1. Purpose

This document defines the shared application-state contract system for `LoFiBox Zero`.

It exists to keep page-local UI state separate from cross-page app truth.
Pages may project or edit shared state, but they must not redefine it locally.

Use this document when the question is:

- what global state is owned by the app rather than by a page
- which state machine owns playback truth, navigation truth, library browse context, queue truth, search truth, source truth, or settings truth
- how pages collaborate through shared state instead of silently duplicating it

## 2. Authority

For final product meaning, use `lofibox-zero-final-product-spec.md`.
For architecture ownership and code placement, use `project-architecture-spec.md`.
For persisted versus runtime-only behavior, use `lofibox-zero-persistence-spec.md`.
For credential references, secret storage, and runtime auth/session handling, use `lofibox-zero-credential-spec.md`.
For scan, build, refresh, rebuild, and readiness rules of the local library index, use `lofibox-zero-library-indexing-spec.md`.
This document defines the shared app-state contracts that sit between those higher truths and the current page implementations.

## 3. Scope

This document covers:

- shared app-state contract conventions
- one file per global app-state machine
- naming rules for shared state events and transitions
- the ownership boundary between page-local state and shared app state

This document does not define:

- page-local selection behavior already covered by individual page specs
- media decode or sink behavior
- DSP internals
- final remote-source page design

## 4. Shared State Contract Template

Each app-state file under `docs/specification/app-states/` should follow this contract shape:

1. `Purpose`
2. `Related Specs`
3. `Owned Truth`
4. `Data Dependencies`
5. `State Enumeration`
6. `Invariants`
7. `Event Contract`
8. `Transition Contract`
9. `Default State`
10. `Error State`
11. `Non-Goals`

## 5. Naming Conventions

Shared app-state files should use these naming rules:

- state names use `STATE_<DOMAIN>_<MODE>`
- events use `EVT_<ACTION>`
- guards are written inline as `[guard: ...]`
- effects are written inline as `effect: ...`

Transition bullets should use this shape:

- `STATE_X + EVT_Y [guard: ...] -> effect: ... -> STATE_Z`
- `STATE_X + EVT_Y [guard: ...] -> effect: ... -> <Target Page>`

Custom event names must remain product- or app-oriented and must not expose backend implementation details.

## 6. Shared App-State Index

The current shared application-state contracts are:

- `NavigationStack`: `docs/specification/app-states/navigation-stack.md`
- `PlaybackSession`: `docs/specification/app-states/playback-session.md`
- `QueueState`: `docs/specification/app-states/queue-state.md`
- `SearchState`: `docs/specification/app-states/search-state.md`
- `SourceState`: `docs/specification/app-states/source-state.md`
- `NetworkState`: `docs/specification/app-states/network-state.md`
- `MetadataServiceState`: `docs/specification/app-states/metadata-service-state.md`
- `CredentialState`: `docs/specification/app-states/credential-state.md`
- `LibraryIndexState`: `docs/specification/app-states/library-index-state.md`
- `LibraryContext`: `docs/specification/app-states/library-context.md`
- `SettingsState`: `docs/specification/app-states/settings-state.md`
- `PersistenceState`: `docs/specification/app-states/persistence-state.md`

## 7. Ownership Rule

Page files under `docs/specification/current-ui-pages/` may:

- read shared app state
- display shared app state
- trigger events that request shared app-state changes

Page files must not:

- redefine shared app-state invariants
- fork separate local versions of playback truth, navigation truth, library context truth, queue truth, search truth, source truth, network truth, or settings truth
- fork separate local versions of library-index readiness truth
- fork separate local versions of credential readiness or session validity truth
- encode cross-page state transitions solely in page-local language when a shared app-state contract already owns that meaning
