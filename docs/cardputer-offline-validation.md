<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Cardputer Zero Offline Validation

Validation date: 2026-11-09. The offline results below were collected while the
device was powered off. Later on the same date the device became available and
hardware validation started. Offline measurements remain host-only evidence;
the subsequent hardware results must be evaluated separately.

## Product artifacts

| Product | Build profile | Package | Presentation |
| --- | --- | --- | --- |
| Cardputer Zero | `cardputer-release` | `lofibox-cardputer-zero` | framebuffer + evdev |
| Debian desktop | `debian-wayland-release` | `lofibox` | native Wayland |

The CMake product profiles force the selected display target and disable the
other target and X11. Cardputer's CLI and APPLaunch entry both reach the same
framebuffer wrapper, even if a desktop session is inherited. Debian's launcher
execs Wayland directly. The packages conflict because they own the same command
and application data. See `packaging.md` for build commands.

The initial offline baseline artifacts were built in an isolated **Debian 12** root:

- `.tmp/offline-artifacts/debian12/lofibox-cardputer-zero_0.3.8-1_arm64.deb`
- `.tmp/offline-artifacts/debian12/lofibox_0.3.8-1_amd64.deb`

`SHA256SUMS` and package dependency summaries accompany them. The ARM64 package
was a deployment candidate until the device's distribution, architecture and
framebuffer format were verified. Keyboard candidates are under
`.tmp/offline-artifacts/keyboard-20261109/` and
`.tmp/offline-artifacts/keyboard-final-20261109/`. The latest keyboard/rhythm
builds are under `.tmp/offline-artifacts/rhythm-20261109/`.
The Cardputer packages use the Debian 13 ARM64 device's 320x170 RGB565 `/dev/fb0`.
The older amd64 Ubuntu 24.04 packages under
`.tmp/cardputer-offline/` are superseded validation artifacts. Package versions
alone do not distinguish these local builds; use the directory and checksums.

The newer icon-control packages are under `.tmp/offline-artifacts/icons-20261109/`.
They replace the two-line screen-key captions with theme-colored icons on all
GUI pages, including top-bar help and text-editor confirmation/cancel controls.
Numeric input, contextual physical-key positions and textual F1/FnH help are
preserved. GUI projections omit redundant ENTER/BACK instruction rows while
terminal projections retain their text. The bounded footer cache still
invalidates on page, theme and play/pause state changes.

Final package candidates must come from a clean `dpkg-buildpackage` build.
A local incremental helper previously reused `debian/debhelper-build-stamp` and
skipped compilation despite newer sources. Those packages are superseded.
The clean-build logs, object/source timestamp checks and matching staged/package
ELF Build IDs accompany the corrected candidates; passing help/install smoke
alone is insufficient to establish that a binary contains the current fixes.

## Hardware results after power-on

The device reports Debian 13.7 ARM64, a four-core Cortex-A53 at 1 GHz,
370,065,408 bytes of OS-visible RAM, the `panel-mipi-dbid` framebuffer and the
internal `tca8418c` keyboard. All 63 MP3 files copied to `/home/pi/Music` matched
the source SHA256 checksums. No TrailMate files or settings were changed.

The keyboard build was installed on hardware. APPLaunch start and long-Escape
return were exercised on the earlier keyboard candidate. Controlled launches
of the later cached-footer and rhythm candidates also use the normal pi user
and 63-track library. The first scan took 63.461 seconds; cached library startup
took 0.851 seconds on the cached-footer build and 0.455 seconds in a later
rhythm development run. These are individual runs, not startup guarantees.

The 4-8 action bar uses each page's contextual definition. Twenty device
keyboard assertions passed, including list paging, one-toggle held play keys,
EQ adjustment, system volume/mute and media keys through help. Search entry
preserved `45678FZXC`. The GUI matrix captures 52 populated/empty page states.
The queue page currently presents current-track/mode status; the playlist
editor's create/remove/reorder/save commands remain unfinished.

The static action bar is cached as a 320x28 canvas. Forty-eight comparable page
captures matched the previous renderer byte for byte. In matched 20-second
idle samples with the real library and the same menu selection, process CPU
fell from 35.16% to 25.36% of one core, a 27.9% relative reduction in these runs.
Maximum RSS was 13,078,528 and 11,431,936 bytes respectively. Sixty menu presses
with varied spacing measured framebuffer-write latency: median/p95/max changed
from 30.629/45.152/47.610 ms to 25.706/41.281/44.334 ms. These measurements exclude
boot and do not measure physical panel scan-out latency.

Those pixel-equivalence captures predate the deliberate icon redesign. The
icon version intentionally changes those pixels and is covered by the later
page matrix, state/theme invalidation, pointer tests and device captures.

On the icon development build with the same empty-playback menu/library state,
another 20-second sample averaged 23.56% of one core (max 30.12%, max RSS
11,829,248 bytes). Sixty varied-spaced menu presses measured median/p95/max
25.178/40.782/40.986 ms to framebuffer write. These short samples do not bound
first-play or metadata enrichment latency; a later first-play run took 3.147
seconds while the keyboard assertions still passed.

### Rhythm indicators

The official schematic's keyboard page identifies all three indicators as
**white single-color LEDs**. RGB color changes require different hardware.
The implementation reuses the existing ten-band spectrum and generates
low/middle/high onset pulses on one latest-value worker at up to 20 Hz. It
validates device-tree register mapping and the internal keyboard name, and
only writes M5IOE1 LED-mode registers BD/BE/BF. No backlight, global PWM
frequency, GPIO direction, reset or power-output control was changed.

A 240-sample, 50-ms hardware readback test observed transitions on all three
lights (14/22/2 transitions in that music excerpt), followed by `[0,0,0]` after
pause. Synthetic evdev Fn state plus the firmware's one-shot/locked LED modes
remained unchanged while music ran, with the other music lights yielded.
This tests software priority, not the entire physical keyboard firmware path.
The separate sequential lamp test also read back every requested state and
restored the original off state. After repeating the sequence with LoFiBox
open and then playing music, the user confirmed that all three white lamps
flash with the music.

The user found the independent onset flashes visually irregular and requested
smooth breathing instead. The generic M5IOE1 library documents PWM channels
for IO9/IO8/IO11, but on this device both multi-byte and single-byte writes to
the first three duty registers read back zero. The existing Linux PWM driver
likewise reported an enabled, 50%-duty channel in sysfs while hardware readback
remained zero. These prototypes did not establish brightness control or a
breathing effect. Saved PWM values, GPIO state and the 20-kHz frequency were
checked after direct probes; kernel PWM1 was unexported after its test. The
backlight channel retained its original bytes. No MCU firmware was changed.
Further firmware/protocol evidence is needed before shipping breathing.
The user subsequently confirmed the native slow mode is uniform flashing.
Read-only analysis of the factory-test bundle's matching minor-version 0x01
image found a zero-fill for read registers 0x1B-0x20 (0x080023BE-0x080023C4)
and three LED mode handlers that reject values above 3
(0x08002C7A, 0x08002C8C, 0x08002CCA). The observed interface therefore does
not establish indicator brightness control. This is evidence from a bundled
same-version image and runtime tests; the live MCU flash was not dumped or
modified. The bundled 0xF8 image also retains the three mode-value upper bounds
of 3. No verified new LED brightness contract or MCU source was found, so no
firmware upgrade was run.
The icon build therefore defaults the music flash pattern to off; it remains
explicitly available with `LOFIBOX_RHYTHM_LIGHTS=1`. Normal keyboard indicators
remain under the existing firmware/driver.

Subsequent source-free reverse engineering produced an isolated 1,016-byte
PWM firmware prototype and 14 passing groups of instruction-level simulations.
It opens a private PWM1-3 interface while preserving the simulated backlight
fields, yields native keyboard modes, and expires stale requests. This does
not establish hardware breathing or safe deployment: the full power FSM,
physical timing and bootloader recovery remain unverified. No firmware was
flashed and the installed application is unchanged. See
[the reverse-engineering report](cardputer-ioe1-reverse-engineering.md) for the
decoded addresses, exact artifact/checksum and validation boundaries.

The final icon Cardputer package was installed successfully and launched from
the actual APPLaunch menu. Its live ELF Build ID matched the package
`18453c3672dd95d7c3ead0c37400e071e0916904`; the running process was a child of
the APPLaunch shell and the launcher service remained active. The selected
track and saved position were restored, with the 63-track library ready and
`LOFIBOX_RHYTHM_LIGHTS=0`. Playback was left running for the user. Firmware and
lamp tests had already restored all three lamp mode bytes to zero.

In a reverse-order playback A/B after a ten-second warm-up, two 20-second
samples measured 47.39% of one core with music lights off and 48.35% on,
including the audio children. Maximum RSS was 75,722,752 and 75,419,648 bytes.
The observed CPU difference is 0.96 percentage points of one core; it is not
a guaranteed upper bound. An earlier short A/B included an artwork ffmpeg burst
and is retained as startup evidence rather than attributed to LED processing.

Rhythm pattern, invalid-value handling, steady-tone settling, publication under
a blocked output callback, latest-value coalescing and normal cleanup are
covered by the new smoke test. The two Linux profiles passed 102/100 tests;
six input/render/worker tests passed ASan/UBSan, and the rhythm worker test
passed ThreadSanitizer with process-local ASLR disabled for this host runtime.

### Unresolved device findings

The user subsequently confirmed normal speaker music output. The later
resource build also fixes the synchronous local metadata/artwork playback path,
duplicate audio process/FD cleanup, child FD inheritance and runtime socket
worker lifecycle. See [the resource validation report](cardputer-resource-validation.md)
for current artifact identities, real-device timings, descriptor observations
and the completed ten-minute playback sample.

- `play <path-or-url>` was reproduced as falling through to ordinary play/resume.
  The subsequent input packages correct this and pass native background input
  and real-decoder checks; they have not replaced the device's installed build
  while the user is using another foreground app. See
  [runtime input validation](cardputer-input-validation.md) for artifact identities
  and its historical acceptance boundary. The newer
  [stream validation report](cardputer-stream-validation.md) records verified
  stream seek/replay, stop responsiveness and numeric socket fixes; decoder
  startup and foreground hardware acceptance remain open.
- Real remote-service behavior, a long audible playback soak, physical panel
  latency and all physical modifier sequences still need separate acceptance.

The historical offline sections below describe the earlier baseline. Their
device-off and not-installed statements refer to that baseline, not these
later hardware sessions.

## Fixed performance issues

The framebuffer presenter previously cleared the entire mapping and converted
and wrote every canvas pixel at every 33 ms tick, including unchanged pages.
This also cleared unrelated virtual framebuffer pages and row padding.

The new `FramebufferBlitter`:

- initializes only the visible viewport on its first frame or canvas resize;
- compares against its previous logical pixels and skips unchanged rows;
- converts and copies only the changed horizontal span in each changed row;
- builds channel conversion lookup tables once instead of dividing per pixel;
- preserves offsets, stride padding, virtual pages, integer scaling and centering;
- validates the mapping and packed truecolor 16/24/32-bit channel layout.

It adds approximately 218 KiB of presentation history, channel tables and row
scratch storage for a 320x170 RGB565 target. The renderer requires exclusive
ownership of the visible framebuffer while the application is running.

The common GUI runner also used to catch up on missed frame deadlines, producing
back-to-back updates after slow rendering or I/O. It now skips missed time slots
and waits for the next available slot. This applies to both product artifacts.
Normal update cadence, playback processing and animation semantics remain intact.

This does not eliminate the CPU work of rendering the logical canvas at 30 Hz.
Full application CPU profiling is still required on the device before deciding
whether to add further render invalidation or change animation cadence.

Other fixes address work shared by GUI, TUI, CLI and WebUI:

- Tag-writer capability checks previously spawned Python and imported Mutagen
  on every full runtime snapshot, while holding the runtime command lock. Each
  writer instance now probes once. A regression checks 1,000 availability queries
  produce exactly one initial probe; actual tag writes still execute normally.
- Track enrichment previously created and retained one joinable thread per
  request. A single worker now keeps only the newest queued job, cancels obsolete
  work and bounds unpublished results. Provider exceptions no longer terminate
  a worker. Cancellation propagates to captured helper processes; Linux helpers
  have their own process group and are reaped after cancellation. Playback's PCM
  subprocess APIs are unchanged. Enrichment curl calls have a 3-second connect
  timeout and 8-second total timeout.
- Shared metadata/lyrics/identity cache entries were accessed through mutable
  references from independent threads. Each provider now uses a private copy and
  merges changed fields under a short mutex. File reads/writes are serialized
  separately. Network requests and decoding never hold the entries mutex.
  Nested identity lookup results survive an outer metadata request's commit.
- TUI ingestion keeps the newest complete snapshot rather than up to 256 full
  snapshots. Its disconnected retry wait now wakes immediately on exit.
- HTTP reads now handle a body arriving after the headers, enforce a bounded
  request size/deadline and reject invalid lengths. At most eight managed workers
  serve HTTP/WebSocket connections; shutdown joins them. Partial sends, closed
  peers and SIGPIPE are handled. Each WebSocket computes its own event cursor,
  so two clients receive the same changes; ping/pong and peer close are exercised.
- WebUI runtime adapter access is synchronized and returns snapshot/error copies.
  CLI numbers reject overflow, NaN, infinity and trailing garbage; HTTP integer
  parsing rejects overflow and fractional/trailing data.
- A concurrent library replacement/WebUI read reproduced a heap-use-after-free
  under ASan. GUI entry points, the runtime command bus and external library
  queries now use the same controller-state guard. Artwork requests copy the
  selected record and release that guard before file/decoder/network work.
- Library sort comparisons previously did linear ID lookups and repeated case
  conversion. They now sort record/key pairs with each key prepared once.
  Song/playlist row construction indexes IDs once, classification counts are
  built in one pass, and unrelated pages no longer compute playlist/album views.
  The six sort modes and playlist ordering/filtering semantics are preserved.
- Artist/album enrichment reserves rate-limit slots under a separate short mutex
  and waits outside it. Wikipedia query titles are URL encoded. WebUI cached
  artwork reads reject invalid sizes and files above 8 MiB before allocating.
- WebUI EQ sliders previously lost their event handlers after a live update
  rebuilt their DOM. They now retain their nodes, handle pointer capture for
  mouse/touch, flush the final drag value and support keyboard adjustment.
  The enable button dispatches real enable/disable commands and retains the
  latest pending user intent until a corresponding server version confirms it.
- Library navigation no longer embeds metadata in JavaScript string literals.
  Quotes, backslashes and Unicode survive navigation and Play All commands;
  same-name albums are filtered by both album and artist. Concurrent library
  loads are coalesced, old responses are invalidated and enrichment results are
  applied only to the current detail view, including cached results on reentry.
- WebUI renders the active visible page, keeps stable lyrics/EQ DOM and stops
  animation callbacks in hidden tabs. Library/queue rows load in batches of 100;
  search still includes the full loaded catalog. Queue/settings/source/diagnostic
  payloads are sent on their change events rather than every spectrum update.
  Unavailable lyrics/spectrum and an empty runtime clear the previous content.
  The main content scrolls within a bounded viewport so long detail/diagnostic
  pages keep the bottom navigation visible; the browser checks all five tabs.
- HTTP workers propagate cancellation to enrichment helpers and limiter waits.
  A real helper that sleeps eight seconds reproduced delayed shutdown; the
  regression now requires shutdown within one second after helper startup.
- `runtime settings apply ...` previously matched the settings-query branch
  before command dispatch and returned success without applying anything.
  The query now requires exactly `runtime settings`; an actual socket CLI test
  checks apply followed by a settings query.
- Wikipedia summaries now use the host JSON string decoder, including spaced
  field separators and Unicode escapes. That shared decoder now combines valid
  UTF-16 surrogate pairs into UTF-8 and rejects malformed/truncated pairs instead
  of emitting invalid UTF-8. Tests cover Chinese, musical-note emoji, the highest
  Unicode scalar, escaped punctuation and malformed/truncated responses.

## Offline measurements

Command: run `lofibox_framebuffer_blitter_smoke --benchmark` from a Release build
with testing enabled. It compares against the original full-clear/per-pixel
algorithm and checks the resulting pixels. Each case runs 1,000 frames after
warming the new presenter. The fixture uses a 320x170 logical canvas in a
320x240 RGB565 **RAM-backed** framebuffer, so it is not a panel driver benchmark.

| Scenario | Original us/frame | New us/frame | Original bytes/frame | New bytes/frame |
| --- | ---: | ---: | ---: | ---: |
| Static page | 403.73 | 4.17 | 262,400 | 0 |
| One changing pixel | 402.34 | 4.49 | 262,400 | 2 |
| Full changing canvas | 414.38 | 125.25 | 262,400 | 108,800 |

These are x86_64 host microbenchmark results from one run. They describe the
presentation stage only and do not predict whole-app CPU percentages or device
frame rate. On a 320x170 physical panel, the original path would write 217,600
bytes/frame and the full-changing new path 108,800 bytes/frame after initialization.
Kernel damage tracking may expand submitted bytes to pages or dirty lines, so
the byte count is not an assertion about bytes actually transferred over SPI.

A separate real-process integration fixture ran the product with an ioctl
adapter for a RAM framebuffer, an evdev input FIFO, real ffmpeg decoding/DSP,
and a paced PCM sink. HTTP snapshot round trips for 100 requests on the same host:

| Snapshot query | Before capability fix | After capability fix |
| --- | ---: | ---: |
| Median | 29.926 ms | 0.512 ms |
| P95 | 35.502 ms | 0.665 ms |
| Maximum | 40.287 ms | 0.992 ms |

An interactive PTY check with the runtime disconnected measured `q` to process
exit at 866.06 ms before the TUI wakeup fix and 3.35 ms afterward. Connected exit
can still wait for the event stream's 250 ms read timeout. These measurements
describe host-side interactions, not a device latency guarantee.

After the cache/TUI changes, the complete process fixture passed again: snapshot
median 0.489 ms, P95 0.608 ms, maximum 0.874 ms and shutdown 216.967 ms in that
run. Evdev navigation reached the actual lyrics page and displayed all three
embedded timed lines; screenshots and raw reports are in `.tmp/surface-check/`.

The later library pass used a generated 10,000-track library with the same
titles/order on this x86_64 host:

| Operation | Before its fix | After its fix |
| --- | ---: | ---: |
| Title sort | 1,352.58 ms | 3.204 ms |
| Songs page render (median of 7) | 31.665 ms | 1.663 ms |
| Move Songs selection | 32.512 ms | 0.789 ms |
| Artists page render (37 artists, median of 7) | 1.754 ms | 0.753 ms |

The page measurements used the optimized sort on both sides to isolate the row
generation change. They exclude hardware presentation/audio/network. The sort
regression also reports 1,000 and 50,000 tracks with `--benchmark`; elapsed time
is recorded rather than used as a machine-dependent pass/fail threshold.

The real-process fixture also passed eight rescans while four HTTP readers
completed 216 track/album/artwork read cycles and playback controls continued.
After that stress phase, 100 snapshot round trips measured median 0.471 ms,
P95 0.610 ms and maximum 1.361 ms; shutdown was 216.874 ms. These are host-only
measurements, not a claim about the Cardputer panel or audible continuity.

A separate Chromium/Weston fixture exercises the real native Wayland executable
and the embedded WebUI at a 390x844 mobile viewport. In a stable one-second
window, EQ/lyrics child-list mutation records dropped from 231/81 to 0/0.
This measures unnecessary DOM replacement, not all browser CPU work.
An injected 10,000-track frontend catalog previously created 10,000 DOM rows in
118.3 ms. The batched version initially creates 100 rows; its measured host time
is recorded in the browser report. Show More exposes 200 rows and searching for
the last track still finds it. This is a frontend render fixture, not an
end-to-end 10,000-file scan, full-catalog transfer or hardware latency result.

## Completed validation

- Release Cardputer build: 100/100 CTest cases passed.
- Release Debian Wayland build: 98/98 CTest cases passed.
- Release smoke targets retain assertions (`-UNDEBUG`, or `/UNDEBUG` on MSVC),
  while the product libraries retain optimized Release configuration.
- All 26 GUI page types render at 320x170 with both empty and populated library
  state. The matrix checks selection bounds, help opening/closing and page-stack
  return. It does not claim every real navigation workflow was exercised.
- The real-process fixture exercises library scan/startup, playback clock and
  spectrum, embedded timed lyrics, CLI/WebUI pause/resume/seek and EQ consistency,
  invalid numeric commands, all ten TUI views, interactive TUI transport keys,
  tab navigation, terminal resize, evdev input and clean shutdown. Timed lyrics
  are embedded in FLAC; this is not an LRC sidecar support claim.
- Real socket tests cover segmented HTTP bodies, concurrent slow/healthy clients,
  invalid lengths, two WebSocket subscribers, ping/pong, bounded worker count,
  idle-client teardown, cancellation of an active captured helper and restart.
- An opt-in Chromium test uses an isolated profile, headless Weston, real ffmpeg
  and a paced PCM sink. It covers all five browser tabs, play/pause, EQ live
  updates, enable/disable, rapid toggles, mouse/touch/final-drag/keyboard input,
  album artist filtering, escaped metadata, Play All, cached enrichment, batched
  lists/full-catalog search, external queue/settings updates, unavailable lyrics,
  two live browser clients, hidden-tab animation suspension, connection status,
  WebSocket reconnect, player restart and stale artwork clearing. It asserts no
  uncaught browser exceptions and records native Wayland surface commits.
  Screenshots/reports accompany the candidates; the fake metadata helper only
  supplies fixture summaries, so this is not a real online-provider test.
- ASan/UBSan checks passed for the GUI page matrix, runtime client/server and CLI,
  enrichment worker, captured subprocesses and WebUI server. The cache concurrency
  test and affected worker/process/server tests passed ASan/UBSan after the cache
  change. The cache concurrency test also passed ThreadSanitizer.
- Library sort, row and external-access regressions passed ASan/UBSan. The
  external-access test exercises three simultaneous library readers, a runtime
  reader/command writer, GUI update/render/input, model replacement, actual async
  scanning and a blocked artwork provider. Rendering and EQ must finish within
  500 ms while the provider is blocked. This test also passed ThreadSanitizer,
  including concurrent enrichment limiter access. `setarch x86_64 -R` was used
  for this test process because the local TSan runtime otherwise failed before
  executing the test with an unexpected-memory-mapping error. No system ASLR
  setting was changed. The artwork-size and title-encoding checks passed too.
- New blitter tests compare RGB565, 24/32-bit RGB/BGR, alpha, scales 1/2/3,
  offsets, padding, guard bytes, virtual pages, resizing and invalid mappings.
- AddressSanitizer and UndefinedBehaviorSanitizer passed for the blitter tests.
- The same blitter tests cross-compiled for ARM64 and passed under QEMU with a
  Cortex-A53 CPU model. Emulator timings are not used as performance evidence.
- Frame deadline tests cover ordinary cadence, long stalls and avoiding catch-up.
- Real ffmpeg -> realtime DSP -> ALSA **null** output smoke passed, including
  visualization and live EQ updates. This verifies the host pipeline, not ES8389
  speaker/headphone output or underrun behavior on the device.
- Both earlier Ubuntu amd64 packages built with `dpkg-buildpackage`, installed separately in WSL,
  passed `debian/tests/smoke`, `lofibox --version`, desktop/AppStream validation,
  and uninstalled successfully. Their payloads have the expected single backend.
- The current Debian 12 Wayland amd64 package was installed with its dependencies
  in the isolated Debian 12 root, passed `debian/tests/smoke`, version/TUI help,
  desktop and AppStream validation. The real-browser regression also runs against
  the installed `/usr/bin/lofibox-wayland` binary before uninstalling it with its
  launchers removed; its ELF Build ID must match the package payload.
  The current ARM64 framebuffer package passed ELF architecture, backend payload
  and QEMU Cortex-A53 application/TUI help/version checks. It has not been installed
  on ARM hardware.
- Wrapper checks cover inherited desktop variables, framebuffer/keyboard override
  precedence, appliance WebUI overrides and arguments containing spaces.
- The Python sampler's CPU arithmetic, process descendants and a live CPU load
  were checked; architecture and implementation-placement checks passed.

The plugin runtime smoke previously created its provider inside `assert()`.
Release compilation removed that side effect and made the test fail. It now
creates the provider explicitly and retains its echo/ping checks in Release.

Lintian reported no errors for either validation package. Remaining warnings
include the existing changelog weekday, debug DWZ naming, and missing manpage
aliases for runtime executables. The whole-worktree SPDX script also flags local
AGENTS/CLAUDE/skill files lacking headers; those user instruction files were not
modified. The changed source files have SPDX headers.

GitNexus change analysis against `main` classifies the aggregate change as
**CRITICAL** (230 indexed changed symbols, 91 affected flows at the latest review). Shared subprocess
capture, JSON decoding and cache paths account for the broad scope. The relevant callers and
functional regressions were checked; this classification is not a low-risk
release approval. Newly added files also require direct review because they are
not all present in the existing index. No commit or remote CI run was made.

## Remaining validation and review

- There is no hardware performance or installation result while the device is off.
  ARM64 QEMU help/version checks establish executable loading, not full device
  compatibility or real-time playback performance.
- Native Wayland and real-browser interaction now run in a headless compositor.
  Physical display/input behavior, IME, other browser engines and real-network
  behavior remain unverified; host browser timings do not predict device CPU.
- Real remote-server integrations require configured endpoints and credentials.
  Gated integration tests without those settings do not establish real service
  correctness. Online lookup quality and fingerprint performance need separate
  validation, including poor-network and disconnected scenarios.
- The reproduced library read/replacement race is fixed and covered by sanitizer
  and real-process rescan stress checks above. Additional consumers accessing
  controller references across threads must use the shared state guard; individual
  reference-returning controller methods do not extend protection past a call.
- The Windows subprocess branch was updated for cancellation but no Visual Studio
  installation is available here, so Windows compilation/runtime is unverified.

## Device acceptance when powered on

First record `/etc/os-release`, `uname -m`, `dpkg --print-architecture`, available
RAM/disk, `/proc/fb`, framebuffer geometry/format, input device permissions, ALSA
cards/mixer state, CPU frequencies and temperature. Use the ARM64 Debian 12
candidate only if that architecture and ABI match; otherwise rebuild against
the device's verified distribution and architecture before installing it.

Exercise APPLaunch start/exit/return, keyboard shortcuts and text entry, library
cold/warm scans, local MP3/FLAC playback, pause/resume/seek/next, queue persistence,
EQ/remix, lyrics/artwork, WebUI, and repeated launches. Remote services need real
configured endpoints and should be tested independently of local playback.

For each scenario, find the running process and collect its complete process tree:

```sh
player_pid=$(pgrep -f '^/usr/lib/lofibox/lofibox_zero_device( |$)' | head -n 1)
python3 scripts/profile-cardputer.py --pid "$player_pid" \
  --duration 60 --interval 1 --output idle.json
```

Repeat for playback, paused playback, menu navigation, EQ/remix and library scan;
use distinct output names. Use longer playback runs (at least 10 minutes) to
check memory growth, thermal throttling and audible glitches. The sampler includes
the player and observed ffmpeg/output children, machine CPU/memory, thermal and
frequency sysfs data and pressure metrics. One core equals 100% for process-tree
CPU; RSS sums shared pages per process. Children that start and exit entirely
between polling intervals may be missed. End-to-end key/display latency and audio
underruns must also be observed or instrumented on real hardware.
