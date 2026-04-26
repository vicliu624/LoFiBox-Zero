<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero App State Contract: PersistenceState

## 1. Purpose

- Own the shared runtime truth for persistence hydration, dirty-domain tracking, save status, and repair status.

## 2. Related Specs

- Shared app-state rules: `docs/specification/lofibox-zero-app-state-spec.md`
- Persistence domains: `docs/specification/lofibox-zero-persistence-spec.md`

## 3. Owned Truth

- whether persisted domains have been loaded yet
- which domains are currently dirty
- whether a save is in progress
- whether domain repair was applied during load or save reconciliation

## 4. Data Dependencies

- persisted storage backend results
- domain-level load outcomes
- domain-level save outcomes
- repair results and fallback decisions

## 5. State Enumeration

- `STATE_PERSISTENCE_UNINITIALIZED`
  persistence hydration has not started
- `STATE_PERSISTENCE_LOADING`
  one or more persisted domains are loading
- `STATE_PERSISTENCE_READY_CLEAN`
  persisted domains are loaded and no dirty domains remain
- `STATE_PERSISTENCE_READY_DIRTY`
  persisted domains are loaded and at least one domain is dirty
- `STATE_PERSISTENCE_SAVING`
  one or more dirty domains are being saved
- `STATE_PERSISTENCE_DEGRADED`
  load or save completed only after repair or fallback

## 6. Invariants

- Shared state must not pretend to be durably loaded before persistence hydration is complete or safe defaults are installed.
- Dirty-domain tracking must be explicit.
- Save failure must not masquerade as a clean persistence state.
- Repair or fallback must be detectable rather than silently swallowed.

## 7. Event Contract

- `EVT_PERSISTENCE_LOAD_STARTED`
  - effect: begin persistence hydration
- `EVT_PERSISTENCE_LOAD_COMPLETED`
  - effect: finish hydration successfully
- `EVT_PERSISTENCE_REPAIR_APPLIED`
  - effect: mark that repair or fallback was needed
- `EVT_PERSISTENCE_DOMAIN_DIRTY`
  - effect: mark one domain dirty
- `EVT_PERSISTENCE_SAVE_REQUESTED`
  - effect: begin save for dirty domains
- `EVT_PERSISTENCE_SAVE_SUCCEEDED`
  - effect: clear dirty flags for saved domains
- `EVT_PERSISTENCE_SAVE_FAILED`
  - effect: preserve dirty flags and mark degraded persistence state

## 8. Transition Contract

- `STATE_PERSISTENCE_UNINITIALIZED + EVT_PERSISTENCE_LOAD_STARTED` -> effect: start hydration -> `STATE_PERSISTENCE_LOADING`
- `STATE_PERSISTENCE_LOADING + EVT_PERSISTENCE_LOAD_COMPLETED` -> effect: mark persistence ready and clean -> `STATE_PERSISTENCE_READY_CLEAN`
- `STATE_PERSISTENCE_LOADING + EVT_PERSISTENCE_REPAIR_APPLIED` -> effect: mark that repair or fallback was needed -> `STATE_PERSISTENCE_DEGRADED`
- `STATE_PERSISTENCE_READY_CLEAN + EVT_PERSISTENCE_DOMAIN_DIRTY` -> effect: mark affected domain dirty -> `STATE_PERSISTENCE_READY_DIRTY`
- `STATE_PERSISTENCE_READY_DIRTY + EVT_PERSISTENCE_DOMAIN_DIRTY` -> effect: expand dirty-domain set -> `STATE_PERSISTENCE_READY_DIRTY`
- `STATE_PERSISTENCE_READY_DIRTY + EVT_PERSISTENCE_SAVE_REQUESTED` -> effect: begin save -> `STATE_PERSISTENCE_SAVING`
- `STATE_PERSISTENCE_SAVING + EVT_PERSISTENCE_SAVE_SUCCEEDED [guard: no dirty domains remain]` -> effect: clear dirty flags -> `STATE_PERSISTENCE_READY_CLEAN`
- `STATE_PERSISTENCE_SAVING + EVT_PERSISTENCE_SAVE_FAILED` -> effect: preserve dirty flags and mark degraded save state -> `STATE_PERSISTENCE_DEGRADED`
- `STATE_PERSISTENCE_DEGRADED + EVT_PERSISTENCE_DOMAIN_DIRTY` -> effect: keep tracking dirty domains after degraded outcome -> `STATE_PERSISTENCE_READY_DIRTY`
- `STATE_PERSISTENCE_DEGRADED + EVT_PERSISTENCE_SAVE_REQUESTED` -> effect: retry save of dirty domains -> `STATE_PERSISTENCE_SAVING`

## 9. Default State

- Default persistence state is `STATE_PERSISTENCE_UNINITIALIZED`.

## 10. Error State

- If hydration cannot load a domain truthfully, `PersistenceState` must reflect degraded load or fallback rather than reporting a clean ready state.
- Save failure must leave domains dirty until they are truthfully saved or deliberately reset.

## 11. Non-Goals

- Storage-engine implementation details
- Fine-grained transaction internals
- Page-level save buttons or UI flow specifics
