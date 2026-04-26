<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero App State Contract: PlaybackSession

## 1. Purpose

- Own the shared playback truth used across `Now Playing`, playlist flows, and playback-adjacent UI.

## 2. Related Specs

- Shared app-state rules: `docs/specification/lofibox-zero-app-state-spec.md`
- Final product objects: `docs/specification/lofibox-zero-final-product-spec.md`
- Queue truth: `docs/specification/app-states/queue-state.md`
- Page projections: `docs/specification/current-ui-pages/now-playing.md`, `docs/specification/current-ui-pages/playlist-detail.md`, `docs/specification/current-ui-pages/settings.md`

## 3. Owned Truth

- current playable item
- play versus pause truth
- playback mode truth for shuffle and repeat
- elapsed and total duration when known

## 4. Data Dependencies

- selected playable item or active queue item projected from `QueueState`
- playback timing source
- known duration metadata when available

## 5. State Enumeration

- `STATE_PLAYBACK_EMPTY`
  no active playable item exists
- `STATE_PLAYBACK_PAUSED`
  active playable item exists and playback is paused
- `STATE_PLAYBACK_PLAYING`
  active playable item exists and playback is playing

## 6. Invariants

- Playback state survives page changes.
- `Now Playing`, `Settings`, and playlist-related pages must reflect the same underlying playback-mode truth.
- Shuffle and repeat are playback-session facts, not page-local facts.
- When playback is not empty, exactly one current playable item is active.

## 7. Event Contract

- `EVT_TRACK_SELECTED`
  - effect: set current playable item and start playback
- `EVT_TOGGLE_PLAY_PAUSE`
  - effect: toggle paused versus playing state when playback is not empty
- `EVT_ACTIVE_ITEM_CHANGED`
  - effect: update playback session to reflect the active queue item or selected playable item
- `EVT_SHUFFLE_TOGGLED`
  - effect: update shuffle mode truth
- `EVT_REPEAT_TOGGLED`
  - effect: update repeat mode truth
- `EVT_POSITION_UPDATED`
  - effect: refresh elapsed playback position
- `EVT_PLAYBACK_ENDED`
  - effect: request the next playback outcome from shared queue truth

## 8. Transition Contract

- `STATE_PLAYBACK_EMPTY + EVT_TRACK_SELECTED` -> effect: set current item, reset elapsed time, begin playback -> `STATE_PLAYBACK_PLAYING`
- `STATE_PLAYBACK_EMPTY + EVT_ACTIVE_ITEM_CHANGED` -> effect: set current item from shared queue truth -> `STATE_PLAYBACK_PAUSED`
- `STATE_PLAYBACK_PLAYING + EVT_TOGGLE_PLAY_PAUSE` -> effect: pause playback -> `STATE_PLAYBACK_PAUSED`
- `STATE_PLAYBACK_PAUSED + EVT_TOGGLE_PLAY_PAUSE` -> effect: resume playback -> `STATE_PLAYBACK_PLAYING`
- `STATE_PLAYBACK_PLAYING + EVT_ACTIVE_ITEM_CHANGED` -> effect: replace current playable item and keep playback active -> `STATE_PLAYBACK_PLAYING`
- `STATE_PLAYBACK_PAUSED + EVT_ACTIVE_ITEM_CHANGED` -> effect: replace current playable item and remain paused -> `STATE_PLAYBACK_PAUSED`
- `STATE_PLAYBACK_PLAYING + EVT_SHUFFLE_TOGGLED` -> effect: update shuffle mode -> `STATE_PLAYBACK_PLAYING`
- `STATE_PLAYBACK_PAUSED + EVT_SHUFFLE_TOGGLED` -> effect: update shuffle mode -> `STATE_PLAYBACK_PAUSED`
- `STATE_PLAYBACK_PLAYING + EVT_REPEAT_TOGGLED` -> effect: update repeat mode -> `STATE_PLAYBACK_PLAYING`
- `STATE_PLAYBACK_PAUSED + EVT_REPEAT_TOGGLED` -> effect: update repeat mode -> `STATE_PLAYBACK_PAUSED`
- `STATE_PLAYBACK_PLAYING + EVT_POSITION_UPDATED` -> effect: update elapsed time -> `STATE_PLAYBACK_PLAYING`
- `STATE_PLAYBACK_PAUSED + EVT_POSITION_UPDATED` -> effect: keep elapsed time synchronized when needed -> `STATE_PLAYBACK_PAUSED`
- `STATE_PLAYBACK_PLAYING + EVT_PLAYBACK_ENDED [guard: queue resolves another active item]` -> effect: request queue advance and wait for active item change -> `STATE_PLAYBACK_PLAYING`
- `STATE_PLAYBACK_PLAYING + EVT_PLAYBACK_ENDED [guard: queue resolves no next item]` -> effect: clear active playback session -> `STATE_PLAYBACK_EMPTY`

## 9. Default State

- Default playback state is `STATE_PLAYBACK_EMPTY`.

## 10. Error State

- Invalid playable items `MUST NOT` create fake active playback state.
- If playback cannot be activated truthfully, the session must degrade to `STATE_PLAYBACK_EMPTY` rather than pretending to play.

## 11. Non-Goals

- Detailed buffering or transport internals in the current shared app-state contract
- Output-device error handling details
- Queue ordering and editing semantics beyond the active playable item projection owned by `QueueState`
