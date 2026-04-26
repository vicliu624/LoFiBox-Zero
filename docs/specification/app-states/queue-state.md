<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero App State Contract: QueueState

## 1. Purpose

- Own the shared playback queue truth used across playback continuity, future queue surfaces, and mixed-source sequencing.

## 2. Related Specs

- Shared app-state rules: `docs/specification/lofibox-zero-app-state-spec.md`
- Final product objects: `docs/specification/lofibox-zero-final-product-spec.md`
- Playback projection: `docs/specification/app-states/playback-session.md`
- Streaming queue semantics: `docs/specification/lofibox-zero-streaming-spec.md`

## 3. Owned Truth

- ordered queue item list
- active queue position
- queue origin or construction mode when relevant
- continuity semantics for previous and next traversal

## 4. Data Dependencies

- playable items selected from library, playlist, or remote source
- insertion or replacement intent
- traversal policy inputs such as shuffle or repeat signals projected from playback state

## 5. State Enumeration

- `STATE_QUEUE_EMPTY`
  no queue items exist
- `STATE_QUEUE_READY`
  one or more queue items exist and one active position is defined

## 6. Invariants

- The queue is one shared player model across local and remote content when their semantics overlap.
- When the queue is not empty, exactly one active queue position is defined.
- Queue order and active index are shared truths and must not be duplicated page by page.
- Queue continuity must not fork by source type unless a real product distinction requires it.

## 7. Event Contract

- `EVT_QUEUE_REPLACED`
  - effect: replace the queue contents and establish a new active index
- `EVT_QUEUE_ITEM_APPENDED`
  - effect: append one item to the queue
- `EVT_QUEUE_ITEMS_APPENDED`
  - effect: append multiple items to the queue
- `EVT_QUEUE_PREVIOUS_REQUEST`
  - effect: resolve previous queue position when traversal allows it
- `EVT_QUEUE_NEXT_REQUEST`
  - effect: resolve next queue position when traversal allows it
- `EVT_QUEUE_CLEAR`
  - effect: remove all queue items

## 8. Transition Contract

- `STATE_QUEUE_EMPTY + EVT_QUEUE_REPLACED` -> effect: set queue contents and active index -> `STATE_QUEUE_READY`
- `STATE_QUEUE_EMPTY + EVT_QUEUE_ITEM_APPENDED` -> effect: create queue with appended item as active entry -> `STATE_QUEUE_READY`
- `STATE_QUEUE_EMPTY + EVT_QUEUE_ITEMS_APPENDED` -> effect: create queue with first appended item as active entry -> `STATE_QUEUE_READY`
- `STATE_QUEUE_READY + EVT_QUEUE_REPLACED` -> effect: replace queue contents and reset active index -> `STATE_QUEUE_READY`
- `STATE_QUEUE_READY + EVT_QUEUE_ITEM_APPENDED` -> effect: append one item and keep current active index -> `STATE_QUEUE_READY`
- `STATE_QUEUE_READY + EVT_QUEUE_ITEMS_APPENDED` -> effect: append multiple items and keep current active index -> `STATE_QUEUE_READY`
- `STATE_QUEUE_READY + EVT_QUEUE_PREVIOUS_REQUEST [guard: previous queue position resolves]` -> effect: move active index to resolved previous entry -> `STATE_QUEUE_READY`
- `STATE_QUEUE_READY + EVT_QUEUE_NEXT_REQUEST [guard: next queue position resolves]` -> effect: move active index to resolved next entry -> `STATE_QUEUE_READY`
- `STATE_QUEUE_READY + EVT_QUEUE_CLEAR` -> effect: clear queue contents and active index -> `STATE_QUEUE_EMPTY`

## 9. Default State

- Default queue state is `STATE_QUEUE_EMPTY`.

## 10. Error State

- Invalid queue indices `MUST NOT` leave queue truth partially defined.
- If queue replacement data is invalid, the state must degrade to `STATE_QUEUE_EMPTY` rather than keep ghost entries.

## 11. Non-Goals

- Full queue-editor UI semantics in the current implementation
- Multi-queue profiles
- Persisted queue restoration semantics
