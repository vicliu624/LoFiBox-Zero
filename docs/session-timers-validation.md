<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Session timers: package and device validation, 2026-11-10

The requested close action is **stop playback and exit LoFiBox**. The player
does not shut down the device. Exit and idle-screen timers are independent,
cancelable session settings, with OFF as the startup default.

## Checks completed

- Clean host framebuffer build: 117/117 tests passed. Clean Wayland build:
  114/114 passed. Focused runtime, timer, GUI, TUI and WebUI checks passed after
  the browser fixes; the changed APPLaunch metadata check also passed.
- AddressSanitizer and UndefinedBehaviorSanitizer runs passed for backlight
  transitions and evdev wake-key/held-ESC handling. Backlight tests include
  descriptor counts, restored brightness, timer disable, and idle boundaries.
- A real headless Edge browser controlled the actual Wayland runtime under
  Weston. The same seven checks passed against the package installed in an
  isolated Debian 12 root using its ordinary `/usr/bin/lofibox` entry. Checks
  covered browser arming, fast CLI cancellation, atomic invalid/unsupported
  rejection, form-key isolation, exit during playback, restart resetting OFF,
  and browser reconnection. Audio used original generated sine fixtures and a
  paced dummy PCM sink; this is not physical speaker validation.
- The ARM package's main and TUI help/version entries executed under QEMU.
  Its packaged native MP3 producer passed the PCM/seek/fallback/FD/pause checks:
  48 kHz output, 7.933-second reference, stereo relative RMS error below 0.001,
  and exec fallback preserving the PID without private descriptors. Emulation
  does not establish real-device responsiveness or CPU use.
- The official Store installation-path and root-service checks passed. Payload
  checks verified APPLaunch Exec existence/executable mode, its PNG path and
  dimensions, private helpers, compiled asset/helper prefixes, absence of the
  Wayland runtime in the ARM package, and the current timer WebUI implementation
  in the actual ELF payload. Debian installed CLI/TUI/help and desktop/AppStream
  smoke checks passed in the isolated root.
- Architecture boundaries, implementation placement and the curated release
  source's SPDX check passed. The full accumulated worktree remains large;
  GitNexus classified its aggregate scope as critical, including earlier audio,
  input, rendering and runtime work. This is not a claim that only timer code
  is affected.

## Bugs caught before distribution

Fast cancellation could leave a browser select at an obsolete duration even
though the runtime was already OFF. Reconciliation now invalidates the timer
control cache after a submitted edit. Settings controls are cached separately
from changing diagnostics so refreshes do not rebuild the active controls.
Select/button/form key events no longer trigger global playback shortcuts.

A local continuation package reused a debhelper completion stamp and missed
the newest WebUI code. The build workflow now clears stamps and staging before
continuation, and checks the actual package payload. An APPLaunch-relative icon
path failed standard desktop validation; the entry now uses an absolute PNG
path, also explicitly supported by APPLaunch's packaging guide. Neither rejected
candidate was submitted to the Store or installed on the device.

## Physical device validation

The user restored device access. The first timer candidate, `0.3.8-3`, was
installed through apt and opened from the real APPLaunch icon. The user
confirmed normal screen output and smooth speaker playback of `500 Miles`.
Testing then caught a real handoff bug: a timed exit while blank stopped audio
and reaped both children, but left the resident launcher's backlight at zero.
This candidate was not submitted.

Revision `0.3.8-4` restores brightness during framebuffer backlight destruction
only when this session owns the blank. The regression test also verifies that
external brightness adjustments and externally initiated blanking are preserved.
The changed test passed normally and with ASan/UBSan. The rebuilt ARM package
passed QEMU entry/helper and Store payload checks. The Wayland executable,
launcher and decoder remain byte-identical to the installed browser-tested
payload; the framebuffer-only fix does not affect compositor power handling.

The final ARM64 package SHA-256 is
`1696e536e821970f2b208c793d6cda479f1a57de670395ceae3307a452de0a86`.
The separate amd64 Wayland package SHA-256 is
`f9b628dd16a2f1d2363b742cf31deb52fa97de6d55d62ccdeaeb323cbfc39e42`.

On Cardputer Zero, apt upgraded the package successfully, `dpkg --audit` was
clean, and the actual APPLaunch icon opened the namespaced installed runtime.
The repeated test used the existing 63-track profile and real music. It covered:

- Wake-only numeric input, held/repeated Next, a 2.25-second held ESC, Shift
  and brightness keys. Playback remained active on the same track. The first
  press/repeats did not perform the mapped playback action or exit.
- Eight further blank/wake cycles, with brightness restored to 50 and no
  descriptor growth (the main process settled at 14 descriptors).
- Cancellation of both timers and continued playback beyond the canceled
  exit deadline; disabling a blank screen timer restored the display.
- A five-second stop-and-exit deadline while blank: the main process exited,
  both sampled audio children disappeared, and APPLaunch resumed with brightness
  **50**. The observation including the final one-second cleanup wait completed
  in 6.35 seconds.
- Reopening with both timers OFF. A separate XDG configuration with only two
  original demonstration MP3s also opened through the unchanged APPLaunch
  desktop entry. The user confirmed normal picture and smooth sound there.

Final-revision 30-second observations of normal-profile playback measured the
entire player process tree at 53.74% mean single-core CPU in steady visible
Now Playing (57.13% maximum) and 9.04% while blank (14.01% maximum). Peak summed
RSS was 20.9 MiB and 19.7 MiB respectively. An initial playback/enrichment
interval averaged 60.24%, peaked at 85.57%, and reached 72.2 MiB summed RSS while
transient helpers were active. Summed RSS can double-count shared pages.
After control commands completed, eight consecutive descriptor observations
were all 14; temporary accepted command sockets must be allowed to close before
comparing steady counts. These percentages use one core as 100%; this device
has four cores, so steady visible/blank means correspond to about 13.4%/2.3%
of total CPU capacity. These are observations, not a guarantee for every codec,
network service or library size.

The synthetic keyboard checks use the real evdev path and real framebuffer;
they do not measure a person's physical key-to-photon latency. Speaker/screen
feedback above is the user's physical observation. Commercial remote service
accounts and custom playlist editing are not covered by this acceptance.

The final 72.4-second demonstration was captured directly from this device's
320×170 RGB565 framebuffer at 15 capture frames per second, with its actual
PipeWire output monitor and only the original demo music. Encoding runs on the
development host. It shows actual APPLaunch startup, library selection, playback,
pause/resume, Next, synchronized lyrics, queue, GUI timer values 15M/30S,
canceling both timers, the equalizer, and held-ESC exit back to APPLaunch.
The listing screenshots are real frames from that capture session, after the
embedded cover and lyrics loaded. The separate XDG profile was then removed
from the launcher's runtime environment; normal configuration hashes matched
the before-recording snapshot, and normal-profile playback was restored.

The remaining publication gates are matching published GPL source and the
Store PR's actual CI/review state. Store maintainer approval remains external.
