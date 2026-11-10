<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Session timer behavior

SLEEP TIMER means stop audio and exit LoFiBox; it never requests device
shutdown. Settings offers OFF / 15M / 30M / 45M / 60M / 90M / 120M. A new value
arms a steady-clock deadline from the time the command is applied. OFF cancels
it. Playback pause/resume and track changes do not extend it. A settings update
that omits this field preserves its deadline, including screen-timeout changes.
The runtime checks expiry before updating playback, stops once, and requests
normal application shutdown. The GUI runner checks again after update and
does not draw another frame after expiry.

SCREEN OFF is available only on the framebuffer runtime. Presets are OFF /
30S / 1M / 2M / 5M / 10M. Keyboard press/repeat activity resets its independent
idle deadline, including modifier keys. At expiry the adapter remembers a
nonzero backlight level, writes zero once, suppresses drawing/presentation and
clears the music LEDs. Audio and runtime processing continue. Any key wakes
the display; the wake batch and that key's subsequent repeats/releases are
consumed. ESC used as a wake key does not arm the long-hold exit gesture.
Turning OFF or changing a screen timer while blank also wakes the display.
Exiting while the session owns a blank restores the remembered brightness so
the resident launcher remains visible. A later external brightness adjustment
is preserved; externally initiated blanking is not treated as session-owned.

The framebuffer adapter keeps no sysfs descriptor open. It performs no
per-frame sysfs reads. A failed blank does not suppress drawing, and retries
are limited to once a second. Missing/unsupported backlight devices leave the
display pipeline running. Wayland monitor power management belongs to the
compositor; a positive framebuffer screen timer is rejected there.

All timer values are volatile and reset to OFF on application restart. Timer
commands are validated atomically. The shared parser accepts OFF or a positive
integer followed by S, M or H, bounded to 24 hours. Empty fields leave that
setting unchanged. The transport adds optional screen_timeout and a snapshot
capability flag; older payloads do not cancel or alter the new screen timer.

CLI: `runtime settings apply <output> <network> <sleep> [screen]`.
TUI command palette: `sleep 30m`, `sleep off`, `screen 1m`, `screen off`.
WebUI More: Stop & Exit / Idle Screen Off (where supported).

Offline checks cover deadline boundaries, cancel/rearm, atomic rejection,
independent settings, stop exactly once, transport compatibility, TUI command
validation, wake and held-ESC handling, brightness restoration, and descriptor
recovery. The actual package installation, APPLaunch handoff, speaker feedback,
blank/wake cycles and resource observations are recorded separately in
[session-timers-validation.md](session-timers-validation.md).
