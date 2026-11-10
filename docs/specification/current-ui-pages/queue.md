<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Current UI Page Spec: Queue

The Queue page presents the shared ordered playback queue. Its rows correspond
one-for-one to absolute runtime queue indices, including unavailable entries.
It is entered from Now Playing or the global queue shortcut, with the current
item selected and visible. The active item is marked separately from the user's
selection. Clearing the queue leaves a stable `QUEUE EMPTY` state; it does not
recreate a queue merely because a current track is still playing.
The title reflects an active shuffle/repeat mode so cycling mode gives visible
feedback without inserting a non-playable status row among queue items.

Only the visible row window is prepared for GUI rendering. The page model
retains the complete row count and absolute offset so keyboard navigation,
scrollbar position and confirmation continue to use the shared queue index.
CLI, TUI and WebUI may request the complete queue snapshot; their snapshot is
not replaced by the GUI row window.

- Up/down moves the selection; Page Up/Down moves by a page.
- Confirm jumps to the selected queue item through the runtime command bus and
  opens Now Playing only when the jump succeeds.
- A missing item retains its row/index and displays `UNAVAILABLE`. Confirming
  it must not play another item or change the active queue position.
- Back returns to the previous page. F1 opens this page's contextual help.
- Global playback/media keys retain their normal meanings.

For the Cardputer profile, keys 4–8 and the matching icon bar mean:

| Key | Action |
| --- | --- |
| 4 | Back |
| 5 | Page Up |
| 6 | Play selected queue item |
| 7 | Page Down |
| 8 | Cycle playback mode |

The standard list renderer accepts complete rows or a window with `total_rows`
and `row_offset`. Selection remains absolute in both cases. A windowed render
must match a complete-list render at the same scroll position and theme.

This page does not implement insertion, deletion, reordering, multiple queues,
or persisted queue restoration. Those require separate shared runtime commands
and state contracts; rows must not imply that these operations already exist.
