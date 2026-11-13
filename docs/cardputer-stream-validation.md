<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Stop responsiveness and stream playback validation

The subsequent [native MP3 validation](cardputer-native-mp3-validation.md)
includes these corrections and records the newer candidate packages.

Validation date: 2026-11-10. This follows
[runtime input validation](cardputer-input-validation.md) and
[resource ownership validation](cardputer-resource-validation.md).
The user requested that the current APPLaunch foreground remain in place.
All new device checks therefore ran in the background with private runtime
sockets and a PCM-consuming test sink. They did not access the framebuffer or
physical speakers and did not replace the installed resource build.

## Corrections

The PCM backend's stop method joined decoder/sink cleanup while holding the
runtime state lock. A measured stop command took 499.829 ms, delaying other
interactions. It now signals the existing audio worker and moves its pipeline
into bounded retirement. That worker still owns descriptor closure and child
reaping; backend destruction waits for all remaining cleanup. No additional
reaper thread was introduced. Up to three retiring pipelines after stop use the
same total budget as two retiring pipelines plus one current pipeline during
playback. Capacity is reclaimed before another decoder/sink pair is created,
preserving the six-child limit. Completed entries are pruned without allocating
another retirement vector.

The runtime now retains the original stream URI privately for seeking and
replay. Display snapshots continue to use the separately redacted URI. Stream
seekability honors both resolved-stream flags and the live-stream flag.
On-demand streams and resolved remote library items can seek; live and explicitly
unseekable inputs reject a seek without replacing their playback. Paused active
streams resume their existing backend. Stopped streams restart at zero, natural
EOF can replay from zero, and a failed seek can retry. Returning to a local file
clears the retained stream input. A single direct stream also honors repeat-one
and repeat-all at natural EOF. The runtime's repeat-off action now clears an
active repeat-one mode as well.

Unapplied CLI commands now return a failure exit status: playback/queue failures
use 6, other unapplied commands use 2. A seek outside the current integer-
millisecond clock range is rejected before it can overflow decoder formatting.
Malformed/non-finite numbers continue to fail argument parsing.

The real HTTP fixture uncovered another defect: the runtime number reader
truncated `1e+308` to `1`. This also affected small values such as `1e-9` in
commands and snapshots. Scalar number parsing now accepts complete JSON number
syntax, including exponent notation, and checks full conversion and token
boundaries. Invalid/missing seek payload numbers are rejected instead of
silently becoming zero. A command/query serialization test first failed on the
old reader and passes on the corrected one. The real HTTP run that exposed the
defect is retained separately from the successful final run.

## Regression evidence

- Framebuffer: 109 tests passed. Wayland: 107 tests passed, using a separate
  temporary directory. Integration tests requiring external credentials are
  not evidence of real service acceptance when those credentials are absent.
- The restart/stop stress test first failed on the old synchronous stop. It
  exercises paused stop, 24 rapid replacements, 12 play/stop bursts, the six-
  child limit, cleanup while the backend remains alive, immediate destruction
  after stop, and descriptor reuse under concurrent pipe churn. FD counts return
  to baseline and all children are reaped.
- That test passes ASan/UBSan and TSan, and runs natively on the ARM device.
  TSan uses per-process ASLR disabling only. Its native paused stop took 67 us
  and slowest bounded rapid replacement took 178 ms.
- Stream lifecycle tests cover seek capability guards, stop/EOF replay,
  repeat, failed-start retry, local/remote transitions and remote library replay
  without a synchronous profile resolution. CLI and numeric-wire tests cover
  failed exit statuses, a huge seek while a valid track is already playing,
  unchanged backend start counts after rejection, and exponent round trips.
  These tests pass ASan/UBSan and TSan; stream and numeric checks also pass
  natively on ARM.
- An initial full run failed three old setup waits under concurrent build load.
  They spun for 500 updates rather than waiting for an asynchronous scan in
  elapsed time. The scan, local-root and help-modal setups now wait within a
  three-second deadline and yield between updates. Full suites then passed.
  The initial failed logs are retained.

## Real ARM background checks

The input fixture used the product runtime and real FFmpeg, with a paced test
receiver standing in for pw-cat. One stop took 14.178 ms, compared with the
previous 499.829-ms sample. These are individual local command-return samples,
not acoustic measurements or percentile latency guarantees.

A subsequent four-second MP3 excerpt from the user's music was served by a
loopback HTTP fixture supporting byte ranges. The final run verified decoded
PCM after initial playback, a seek to one second, pause/resume, restart after
stop, replay after natural EOF and repeat-one. Nonzero HTTP byte-range requests
were recorded. Repeat-off cleared the mode, and a huge seek returned exit 6
without replacing playback. The runtime remained alive after stop while its
decoder/sink children were reaped. The harness exited normally; APPLaunch's
MainPID was 170273 before and after.

| Final HTTP fixture action | Local command return |
| --- | ---: |
| Initial input | 43.142 ms |
| Seek | 17.233 ms |
| Pause / resume | 17.986 / 20.999 ms |
| Stop | 17.286 ms |
| Resume after stop | 16.250 ms |
| Play after EOF | 20.920 ms |
| Repeat one / off | 12.763 / 14.038 ms |
| Rejected huge seek | 13.075 ms, exit 6 |
| Final stop | 14.966 ms |

The test sink does not establish physical speaker delay or sound quality. The
earlier user confirmation of normal speaker audio applies to the installed
resource build. It is not automatically acceptance of every later build.

## Artifacts and review scope

Final packages are in `.tmp/offline-artifacts/stream-20261110/`. A clean baseline
build was followed by an explicit CMake rebuild and repackaging of the numeric
wire fix. Source hashes, baseline/final build logs, raw device reports and test
logs accompany the packages. The ARM package executable's Build ID matches the
candidate used for the final background HTTP run.

| Product | Package SHA256 | ELF Build ID |
| --- | --- | --- |
| ARM64 framebuffer | `6c88b5074e167f6dcc449877421cd54c6a542b2e862d3965e52698366acf2866` | `9f45b73c5c5854ff8d5ea7bdddf77ac3520dac77` |
| amd64 Wayland | `211f9a01c1a5976c900a3c1113a52408ef30a94fef89f4a14449b282ab71d67d` | `7a553461ee36cab64636cee8c99267c40dd85b88` |

Payload separation, ARM64/QEMU loading and core smokes, isolated Debian 12
installation, installed-binary smokes, real Weston/Chromium interactions and
uninstall pass. The final browser run recorded 432 Wayland commits with no
console exceptions. Lintian has no errors; existing weekday/debug-name/manpage
warnings remain.

The ARM package SHA256 was checked after transfer to the actual device.
`apt-get --simulate --reinstall` resolved the replacement with no additional
packages or removals. This was a simulation: the installed executable remains
the resource build (`c4a1d8a97cb21312eeaa29b815ba7c729165c789`), `dpkg -V` is
clean, and APPLaunch remains active with MainPID 170273.

GitNexus reports medium impact for the queue playback entry (five direct
callers) and numeric reader (five direct callers; command/query/event flows).
Other indexed methods in this change are low risk. The anonymous host backend
is not indexed; its stop, restart, destructor and child ownership paths were
reviewed directly. The aggregate worktree is classified critical (327 indexed
changed symbols, 121 affected flows, 108 changed tracked files at this review),
including extensive earlier work. New/unindexed files require direct review as
well. No commit or push has been made.

## Outstanding work

Decoder first-PCM startup remains under investigation. The prior low-priority
probe measured 2,802 ms for the first paced decode and 936–1,146 ms in subsequent
runs. It did not show a consistent benefit from simply removing pacing or
bounding probing, so decoder arguments were not changed on that evidence.
Foreground screen/input and physical audio acceptance of these packages remain
pending while the user uses another app. Playlist/queue-page editing, actual
remote service integrations and MCU breathing qualification also remain open.
