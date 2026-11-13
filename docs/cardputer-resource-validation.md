<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Playback responsiveness and descriptor ownership validation

Date: 2026-11-09. This follows the icon/keyboard validation in
[cardputer-offline-validation.md](cardputer-offline-validation.md).
The user has now confirmed that the device speaker plays music normally.
MCU firmware remains unchanged; the experimental breathing-light image has
not been flashed or included in an application package.

## Issues reproduced and corrected

Local playback previously started the backend and then synchronously called
metadata and artwork providers from `PlaybackController::playQueueIndex`.
`LocalOnly` reads can launch ffprobe/ffmpeg and wait for cache ownership. Six
real-device `play --id` commands took 1.8501, 3.9798, 4.2240, 2.1444, 3.4357 and
2.3923 seconds. These commands used track IDs 6, 60, 12, 25, 36 and 6 in order,
with a 300-ms gap. A repeat of an already visited song was still slow.

All local enrichment now runs on the existing cancellable, coalescing worker.
It publishes local tags/artwork before beginning online enrichment. Partial
results do not clear the pending-lyrics state or overwrite indexed lyrics.
Indexed titles retain the existing merge policy. Playback accounting still
happens once per accepted start. The same worker and generation guards prevent
old jobs from replacing the currently selected song.

A syscall trace then showed the command thread and audio worker both calling
waitpid/close for the same decoder/sink. There were duplicate closes returning
EBADF. This is a descriptor-reuse hazard in addition to a source of delay.
The PCM worker now owns reaping and descriptor closure. Control calls send a
nonblocking stop request under a short process-state lock. A paused child is
continued after TERM so it can handle termination. The worker coordinates
cleanup; up to two retired pipelines are retained alongside the current one,
bounding the number of decoder/sink children at six. At that limit, explicit
backpressure waits for the oldest retirement instead of allowing unbounded
process/memory growth.

Before the descriptor inheritance fix, actual ffmpeg/pw-cat processes held
framebuffer descriptors, the runtime listening socket, an inherited inotify
descriptor and old audio pipe ends. The fixes are:

- Host pipes use atomic `pipe2(O_CLOEXEC)` creation.
- Host spawn actions close descriptors above standard input/output/error after
  configuring the child's streams. The legacy audio and plugin fork paths do
  the equivalent before exec. This also excludes non-CLOEXEC descriptors that
  APPLaunch itself passes into the player.
- Plugin pipes also use CLOEXEC. Partial spawn-attribute setup failures release
  initialized actions and pipe ends.
- Runtime sockets and accepted connections use CLOEXEC. The request socket also
  has scope-based cleanup if an allocation/serialization operation throws.
- Runtime connection workers own their sockets. Stop uses shutdown to wake
  incomplete reads/writes, joins workers, then releases the listener. Completed
  workers are joined/reclaimed on subsequent accepts; active clients are capped
  at 16. Shutdown and worker close use the same per-socket mutex to avoid acting
  on a recycled descriptor number.
- Framebuffer open uses CLOEXEC. If ioctl/mmap setup throws, the constructor
  closes the fd explicitly, because the object's destructor will not execute.

The close-from spawn action is available in the Debian 12 build libc. Its
behavior is documented in the
[glibc implementation change](https://sourceware.org/pipermail/glibc-cvs/2021q3/073654.html).
Both products are still compiled in the Debian 12 root and load on their
validated targets. This work does not establish compatibility with older
glibc releases or a Windows build.

## Measured device behavior

After moving local enrichment and fixing PCM cleanup, the same six standalone
play commands took 0.0203, 0.0248, 0.0270, 0.1138, 0.0474 and 0.0561 seconds.
The median is 37.2 ms versus 2.914 seconds in the six-command baseline. These
are command-return measurements, not a measurement of acoustic start latency.
Caches/network/CPU scheduling vary between runs; the blocked-provider and child
lifecycle regressions separately reproduce the removed blocking paths.

The subsequent FD-corrected build was also tested while injecting menu
navigation during twelve track changes. Framebuffer-change latency ranged
from 30.990 to 178.635 ms. The combined command/navigation checks completed
in 41.6 to 191.7 ms. The combined duration includes waiting for the observed
framebuffer change and is not an isolated CLI response time. Pause and resume
commands took 17.8 and 33.6 ms. Panel scan-out latency is not measured here.

Two hundred sequential real runtime queries were issued after warmup:

| Player observation | Before | After |
| --- | ---: | ---: |
| Open descriptors | 12 | 12 |
| Kernel threads | 5 | 5 |
| Virtual memory, KiB | 408884 | 408884 |
| Resident memory, KiB | 16360 | 16372 |

After the rapid track changes, one sample had 13 fds while an asynchronous
metadata helper was active. Virtual memory high-water values can also change
when new worker stacks/malloc arenas are first used; they are not resident RAM.
The subsequent 600.822-second playback observation completed successfully.
All 120 runtime checks reported playing with active audio, across four tracks;
there were no query errors or playback errors and the same player stayed alive.
Descriptors stayed at 12–13 and returned to 12; kernel threads stayed at five.
Player RSS increased from 8,688 to 9,704 KiB, with 9,704 KiB the observed maximum.
Across 542 CPU samples, the player process tree averaged 39.93% of one core,
with a 124.22% peak; whole-system CPU averaged 20.23% of all four cores.
The process-tree RSS sum peaked at 123,371,520 bytes (117.7 MiB); this sum counts
shared pages separately in each process and is not PSS. Minimum available
system memory was 114,188,288 bytes (108.9 MiB). Maximum temperature was 56.382 C.
Swap was already in use before observation (150,515,712 bytes); it ended at
154,030,080 bytes and peaked at 158,826,496. These totals alone do not establish
active paging or a player memory leak. Existing desktop services account for
substantial swapped memory; kernel memory-pressure PSI is unavailable.
Raw samples and the generated summary accompany the resource packages.

The actual running FD-corrected player and its ffmpeg, pw-cat and ffprobe
children were inspected via `/proc`. The three children no longer held the
framebuffer, runtime listener, inherited inotify watch or unrelated old pipes.
PipeWire's own eventfds, sockets and memfds are expected child-owned resources.
APPLaunch's inherited descriptors can remain in the parent player; the child
spawn boundary prevents propagation.

## Regression evidence

- Framebuffer profile: 106 tests passed. Wayland profile: 104 tests passed.
- Three ownership/restart tests pass under AddressSanitizer/UBSan and under
  ThreadSanitizer. The local TSan runtime uses process-local ASLR disabling
  (`setarch x86_64 -R`); system ASLR settings are unchanged.
- The blocked-local-provider test fails against the previous ARM core and
  passes against the fixed ARM core under QEMU. It covers start/pause/next while
  metadata/artwork/online providers are held, early local results, existing
  metadata/lyrics preservation, current-track guards and play count accounting.
- The paused-restart test fails against the previous host backend and passes
  against the fixed one. It runs real child processes and pipes, simulates slow
  termination, performs 24 restarts, verifies the six-child cap and full reaping,
  and checks fd counts after stop. A concurrent pipe-churn thread checks that
  old audio cleanup does not close newly reused unrelated fd numbers.
- The ownership test deliberately opens a non-CLOEXEC parent canary. Capture,
  run, decoder, sink, legacy-audio and plugin helpers must not inherit it.
  One hundred rounds of bad-framebuffer/missing-executable failures leave the
  parent fd count unchanged.
- The runtime socket test covers 200 sequential queries, event stream close,
  incomplete client frames during shutdown, and restart. libc stack/arena
  caches are warmed before retained-virtual-memory growth is checked.
- The three new resource tests also pass **natively on the Cardputer device**.
  Its delayed-child stress test measured a 174-ms slowest restart. The isolated
  socket test grew by one cached stack (8,454,144 virtual bytes), within its
  bounded threshold; descriptor counts after shutdown returned to baseline.
- An initial parallel run of the two product suites collided in a shared
  tag-writeback fixture directory. The Wayland suite was rerun with a separate
  valid TMPDIR and passed. A first attempted TMPDIR override was malformed by
  shell quoting; that invalid run is retained in logs and is not counted as
  application regression evidence.

## Current product artifacts

Both clean-built packages are in `.tmp/offline-artifacts/resources-20261109/`.

| Product | Package SHA256 | ELF Build ID |
| --- | --- | --- |
| ARM64 framebuffer | `b1eadcb99db474d6703f2ceb7a5f57c3ba4e1a58a7c21bf59cd1c0481ce64552` | `c4a1d8a97cb21312eeaa29b815ba7c729165c789` |
| amd64 Wayland | `80a02eb4e42ee7154567919819e576b47dafe0ab74397bc35a631018bfc19b8f` | `63963b72962c4ff9aa8ac3a1b5b87e3be9cc38e6` |

The Cardputer package was reinstalled on the actual device. `dpkg -V` is clean;
both the installed executable and the live process match the package Build ID.
The player remains an actual APPLaunch child (PID 42512 at this observation).
The package install did not replace the running process with a different build.

Payload separation, ARM64/QEMU loading and core smokes, isolated Debian 12
Wayland installation, installed-binary smoke, real Weston/Chromium interaction
and uninstall passed. The installed browser run recorded no console exceptions.
Lintian reports no errors; pre-existing changelog/debug-name/manpage warnings
remain. Earlier `icons-20261109` and `playback-20261109` artifacts are historical.

GitNexus classifies the **aggregate worktree** against main as critical:
256 indexed changed symbols, 111 affected flows, 89 changed tracked files.
This includes earlier work beyond these resource fixes. New/unindexed symbols
also received manual review and direct tests. Shared output capture itself is
critical (8 direct callers, 12 affected flows), and generic run is high risk
(6 direct callers, 4 affected flows). These were reported before edits.
There is no commit, push or remote CI run.

## Remaining acceptance

The 600-second read-only playback/CPU/RSS/fd/thread observation is complete as
recorded above. Physical speaker output was confirmed by the user; acoustic glitches
during all scenarios and physical panel latency still require appropriate
observation. Source-free breathing firmware qualification, the reproduced CLI
path-opening defect, unfinished playlist editing/queue interactions and real
remote-service integrations remain separate outstanding work.

## 2026-11-13: persistent controls and battery mode (0.3.8-5)

This follow-up implements Settings switches for WebUI, the shared external
CLI/TUI runtime server, GUI spectrum, music LEDs, LED animation, and long battery
mode. Preferences use an atomic replacement file under the XDG configuration
directory. Battery mode overrides the individual choices without discarding
them. Its static page redraws run at 2 Hz; input, playback and timer handling
retain their 33-ms loop. Input and wake force an immediate redraw. There is no
additional decoder for music lights, and PCM pacing/output buffering is unchanged.

Unused GUI pages do not each run a renderer. The main measured difference from
the official Music player is LoFiBox's continuously rendered visualization.
Official Music's static playback page measured 5.72% of one core in an earlier
15-second observation, whereas LoFiBox's dynamic page measured 35.52% in an
earlier 30-second observation. These are different presentation features and
measurement windows, not a feature-identical benchmark.

After the user's reboot, ordinary APPLaunch startup and the existing library's
Sum 41 `Pieces` were used for the following approximately 30-second samples.
Values include the player and its audio descendants. One full CPU core is 100%;
the board has four cores. RSS sums can count shared pages more than once.

| State | Mean CPU, one core | Maximum CPU, one core | Median summed RSS |
|---|---:|---:|---:|
| Original 0.3.8-4, dynamic Now Playing | 36.76% | 45.37% | 24.95 MiB |
| Candidate, dynamic Now Playing after startup | 32.83% | 37.60% | 22.35 MiB |
| Candidate, long battery, visible static player | 9.08% | 14.46% | 22.47 MiB |
| Candidate, long battery with screen blank | 7.67% | 9.77% | 22.62 MiB |
| User's final all-features-OFF state, battery mode ON | 9.24% | 11.97% | 24.73 MiB |

The normal-page observation is about 11% lower CPU than the old version, and
battery mode is about 72% lower than the candidate's normal dynamic page. These
are sampled results rather than guaranteed battery-runtime or latency figures.
Separate initial/track-change samples contained ffprobe helpers and higher
transient CPU/RSS; those intervals are retained in the private evidence rather
than presented as stable playback. Short periodic reachability probes can also
appear among descendants. Battery mode pauses online enrichment, while local
tags, directory/embedded covers, cached lyrics and connectivity status remain.

The user's final configuration was `lofibox-preferences-v1 32`: all five optional
features OFF and battery mode ON. A separate 20-second system-wide observation
measured 12.19% total four-core CPU. The player itself used 2.65% of one core,
the native MP3 decoder 4.70%, and pw-cat 1.95%. APPLaunch used 5.25%, PipeWire
5.10%, and pipewire-pulse 5.00%, each on the single-core scale. The player tree's
9.24% mean corresponds to about 2.31% of total CPU capacity. Available system
memory was about 135 MiB. A five-second check observed zero swap-ins, swap-outs
and major page faults; allocated historical swap is not evidence of active paging.

Live checks through the real framebuffer/input path verified:

- WebUI OFF closed its listening port while the GUI/runtime remained alive;
  CLI/TUI OFF removed its Unix listening socket while WebUI remained available.
  Each could be restarted, and battery mode closed both.
- Stable normal descriptors were 14. Battery mode had 12 descriptors and six
  threads, compared with eight threads before the mode change. A later reopened
  normal session returned ten consecutive descriptor samples of 14. A just-served
  connection can temporarily add a worker, so thread/FD samples require settling.
- The GUI spectrum could be hidden independently. LED OFF and steady mode were
  exercised; the default animation still follows the accepted rightmost spectrum
  column. Keyboard modifier indications retain priority.
- The new GUI **10S** screen choice blanked backlight brightness from 100 to 0
  after 11.5 seconds without stopping the player. Space woke to brightness 100
  and was consumed: subsequent runtime status remained PLAYING/ACTIVE.
- A 13-second API stop-and-exit timer with a 10-second idle screen timeout first
  blanked the display, then exited normally, restored brightness 100, and reaped
  the three sampled child processes. Reopening through the installed APPLaunch
  icon showed both session timers OFF.
- Saving WebUI OFF, exiting and reopening preserved OFF despite the launcher's
  `LOFIBOX_WEBUI=1` default. All-feature restoration was tested before the user
  selected the final all-OFF configuration; that user configuration is preserved.

The original 0.3.8-4 headphone issue did not reproduce: the user confirmed correct
headphone/speaker routing after reboot and two hotplug cycles. No audio-routing
change was needed for those results. On 0.3.8-5 the user again confirmed normal
picture, sound, smooth playback and headphone switching. ALSA jack/DAPM state,
not the sometimes-stale PulseAudio active-port label, was used for routing checks.

Internet status is now cached on a background worker instead of parsing the
route file on GUI reads. Route loss is checked approximately once per second;
bounded metadata-host probes run every 30 seconds online or 15 seconds offline.
Once offline is detected, online process capture is cancelled, descendants are
killed/reaped, and failed network work is not cached as a completed metadata miss.
Current-track enrichment can resume on connectivity recovery. The direct CLI
and WebUI enrichment paths use the same cancellation policy; local/cache reads
remain available. Detection is asynchronous, not an instantaneous WAN guarantee.

Validation passed 121 framebuffer-profile and 118 Wayland-profile release
tests, including live PCM visualization disable/restore without pitch, sample
count or playback-clock changes. Focused coverage includes 20 interface
stop/start cycles with exact FD recovery, offline cancellation/recovery, worker
shutdown, steady LED behavior, preferences persistence, and cancellation after
a helper closes stdout before exiting. Canvas copying/filling also passed an
independent pixel reference plus ASan/UBSan; the full Now Playing render was
byte-identical before/after the copy optimization.

The final resource-only adjustment adds 10S to the WebUI dropdown as well as the
GUI. Six affected WebUI/timer/interface tests passed for each profile after that
adjustment. The final Wayland package connected to Weston headless, rendered
33 commits, answered a runtime query and exited normally. The ARM executable
also loaded under QEMU with Debian 12 libraries. The device's current music
session and saved all-OFF preferences are preserved while this resource update
is installed; its embedded WebUI dropdown loads on the next application start.

Final private candidate artifacts (not replacements for the published 0.3.8-4):

- `lofibox-cardputer-zero_0.3.8-5_arm64.deb`: SHA-256
  `45685968cf120ec2ec18430b6c5e882eb3fca2e863b52c17fe79232a1514bca2`.
- `lofibox_0.3.8-5_amd64.deb`: SHA-256
  `cba7cd8d5215687c92f0b642f043bc2c9f29ef756aa2f78da07dc93b259d3b82`.

Private raw samples, screenshots and control results are under
`.tmp/candidate-0.3.8-5`. The existing Store PR/release assets are unchanged.
