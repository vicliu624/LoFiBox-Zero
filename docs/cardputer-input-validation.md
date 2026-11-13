<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Runtime playback input validation

Validation date: 2026-11-10. This follows the resource ownership corrections in
[cardputer-resource-validation.md](cardputer-resource-validation.md).
The subsequent [stream validation report](cardputer-stream-validation.md)
records corrected stop responsiveness, stream seek/replay and numeric wire
handling in newer artifacts. The input-build measurements below are historical.

## Corrected behavior

The advertised `lofibox play <path-or-url>` previously discarded the positional
input and submitted generic play. In addition, a leading `--json` could consume
the command name as its value. A new typed `PlaybackOpenInput` command now
crosses the existing Unix runtime socket and command bus.

- The CLI resolves relative paths in its own working directory. Existing audio
  files with spaces, quotes and UTF-8 names, and local `file:` URIs including
  percent escapes and the localhost authority, are supported.
- The running session validates the input again before changing playback.
  Missing files, directories, malformed file URIs, unsupported URL schemes,
  unknown play options and conflicting actions produce errors. They do not
  silently start another song. `--` supports filenames beginning with a dash.
- A file outside configured library roots receives a normal track identity,
  queue entry and asynchronous metadata enrichment. Opening a file also selects
  it when the GUI was previously showing a different album or genre.
- A scan started before the open cannot replace that file with a different
  track using the same numeric ID. Explicit file identity and play counters
  survive subsequent scans, supplementary files are deduplicated against normal
  roots, and deleted files are removed on refresh. Default library roots are
  still used when no explicit roots were supplied.
- Reading tags/artwork and probing network resources remain outside the input
  command. A gated metadata test verifies that submitting an unindexed input
  returns within 300 ms while the tag provider is blocked. Its filename title
  is subsequently replaced with the actual tag title.
- Direct URLs use the existing stream backend. Snapshot display and playback
  start/failure log messages omit URL userinfo, query parameters and fragments.
  This is not a claim that arbitrary third-party decoder error text is filtered.

## Evidence

The new `runtime_play_input_smoke` first failed on the previous implementation.
The fixed test covers the command socket, fake recording backend, local path
normalization, URI handling, selector rejection, background tag delivery,
in-flight scan ID collisions, play counters, rescan deduplication, file removal
and stream display redaction. The complete framebuffer suite passes 107 tests;
the Wayland suite passes 105, with separate temporary directories.

The input test passes AddressSanitizer/UBSan and ThreadSanitizer. TSan uses
per-process ASLR disabling only. The same test also passes natively on the ARM
device, without a GUI or physical audio sink. APPLaunch's MainPID was 170273
before and after that test.

A separate native background harness then used the product runtime and real
FFmpeg with a paced PCM-consuming stand-in for pw-cat. It opened a copy of an
actual user MP3 by relative UTF-8 path and file URI, sought locally, paused and
resumed, and decoded the same MP3 over a loopback HTTP server. Audio progress
and visualization frames were observed; stop returned to idle. URL credentials
and query tokens were absent from snapshots and the playback runtime log.
The harness exited normally and APPLaunch's PID stayed 170273. This exercised
real decoding without accessing the framebuffer or physical speakers.

The background run measured command returns of about 114 ms for the first file
open, 48 ms for the file URI and 18 ms for HTTP input. Local seek took about
20 ms; pause/resume commands took 13–20 ms. These are command acknowledgements,
not first audible output. Stop took about 500 ms and requires further
responsiveness work. Runtime log timestamps also show seconds between input
submission and the first processed PCM frame, requiring separate decoder-startup
investigation. The test HTTP server reported a connection reset when the decoder
was stopped before the complete MP3 transfer; the harness and all required
decoding checks completed successfully.

A subsequent nine-run, low-priority decoder-only probe used the same 11,135,031-
byte MP3, no physical output and no GUI. The first default-paced run reached
PCM in 2,802 ms; subsequent runs across default pacing, no pacing and bounded
probing were 936–1,146 ms. These sequential runs do not isolate cache warmth
from scheduling or decoder initialization. Neither removing `-re` nor bounding
probing showed a consistent improvement in the warm runs, so the product's
decoder arguments were not changed on this evidence alone.

The two packages in `.tmp/offline-artifacts/input-20261110/` were built from a
clean baseline followed by an explicit CMake rebuild and repackaging of the
final file-removal correction. This final rebuild was run explicitly; relying
only on `dpkg-buildpackage -nc` could reuse debhelper's completed build stamp.
Source file hashes, baseline and final build logs accompany the packages.

| Product | Package SHA256 | ELF Build ID |
| --- | --- | --- |
| ARM64 framebuffer | `6aa72d461187daca54d8f0a3ad604b4f76e7a9abada4711b63886270723e3f58` | `be5eff77be3fbc9906b882045682039af62c5590` |
| amd64 Wayland | `70806e9591ebfb4702d74c7286763126d39c9c6ef63f967a54d6197ac31b2c10` | `a5acd4429e5913505c79a62a792678923d8a6a6c` |

Payload separation, ARM64/QEMU loading and four core smokes, isolated Debian 12
installation, installed-binary smokes, real Weston/Chromium interactions and
uninstall pass. The browser recorded 420 Wayland commits and no console
exceptions. Lintian has no errors; the existing weekday/debug-name/manpage
warnings remain.

## Device foreground and remaining acceptance

The user requested that the current APPLaunch foreground remain in place.
Accordingly these new packages have not replaced the installed resource build,
and no foreground LoFiBox restart or screen/input acceptance was performed for
this input correction. The background real-decoder test above is separate
from the GUI and physical speaker checks. The earlier user confirmation of
normal speaker output and the completed resource soak apply to the resource
build, not automatically to every later package.

At the time of this input build, direct-stream seeking and restart after stream
EOF were unfinished. Newer stream artifacts correct and verify these paths as
recorded in the subsequent report linked above.
Actual remote-service credentials, catalog integrations and every network
protocol are not established by a direct HTTP fixture test. Playlist editing,
queue-page interactions, physical panel latency and MCU breathing qualification
also remain outstanding parts of overall acceptance.
