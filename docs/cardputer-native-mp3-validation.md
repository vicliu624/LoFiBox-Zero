<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Native MP3 startup validation

**Candidate withdrawn after real playback on 2026-11-10.** The helper emits
44.1 kHz PCM while the actual product pipeline consumes 48 kHz. Earlier producer
tests compared against a 44.1 kHz reference and therefore missed this integration
error. The live player showed under-runs and an early endpoint at 152.736 s for
166.373 s of decoded PCM, matching the rate error minus the output-latency hint.
These packages are rejected. The device was restored to resource build
`c4a1d8a97cb21312eeaa29b815ba7c729165c789`; correction and full-duration acceptance
are documented in [PCM and backlight regression validation](cardputer-audio-pcm-validation.md).
The measurements below describe the withdrawn implementation.

Validation date: 2026-11-10. This candidate includes the preceding
[stream/stop corrections](cardputer-stream-validation.md) and adds a private
native MP3 PCM producer. The user requested that APPLaunch remain in the
foreground. All device tests used private sockets and a paced discard sink;
they opened no framebuffer, input device or physical audio output. APPLaunch
MainPID remained 170273. The installed resource build remains
`c4a1d8a97cb21312eeaa29b815ba7c729165c789`, with a clean `dpkg -V` result.

## Implementation and scope

The Linux products include `/usr/lib/lofibox/lofibox-mp3-decoder`, linked to
libmpg123. Local 44.1 kHz stereo MP3 can produce float PCM at its original rate.
ReplayGain adjustment and automatic resampling are disabled. Other rates,
channel counts and unverified streams use the existing FFmpeg argv. Non-MP3
and network inputs retain their existing path. The helper replaces itself with
FFmpeg on fallback, preserving the PID, process group and PCM pipe, with no
additional child or reaper thread. The existing audio worker retains cleanup
ownership.

The input FD is opened with CLOEXEC and has a separate RAII owner, destroyed
after the mpg123 handle and before fallback exec. The API specifies that the
caller retains ownership of descriptors passed to
[`mpg123_open_fd`](https://www.mpg123.de/api/group__mpg123__input.shtml).
The helper allows native formats and checks the negotiated result rather than
forcing a sample rate; the API describes forced-rate conversion in its
[output format documentation](https://www.mpg123.de/api/group__mpg123__output.shtml).

Only a complete Xing/Info byte extent verified against file size is accepted
for native decoding. Bounded pread calls inspect the ID3v2 header, first MPEG
header and trailing ID3v1/APEv2 metadata; they do not scan or allocate the whole
file. APEv2 header/footer size and flags are consistent with the
[Mutagen implementation](https://github.com/quodlibet/mutagen/blob/master/mutagen/apev2.py).
Among the user's 63 files, 49 have matching extents and take the native route;
14 have inconsistent extent records and take FFmpeg. All 63 produced PCM.

The native producer retains real-time pacing. Its monotonic clock anchor resets
after a long pause/backpressure stall instead of emitting a catch-up burst.
Its 8192-byte buffer and process count remain bounded. Native seeking uses
16 pre-roll frames to rebuild decoder history before the requested sample.

## Defects caught during development

The first seek comparison used FFmpeg's fast pre-input `-ss` as the reference.
That reference itself contained a silent prefix after seeking. Full decoding
followed by sample-accurate trimming exposed a separate small transient in
mpg123's default pre-roll. Increasing pre-roll to 16 eliminated the transient
in the synthetic fixture. The quality check now compares against full decoder
history, rather than that fast-seek output.

Concatenating 44.1 kHz stereo MP3 with mono or 48 kHz content exposed another
issue: changing producers after partial output could introduce discontinuity.
The byte-extent guard now routes these fixtures to FFmpeg before any native
output. Their final sample counts and PCM match the existing FFmpeg path.
Missing Xing information, renamed WAV input, invalid input and unsupported
formats also retain FFmpeg behavior.

The device scripts initially found two fixture issues: a direct probe waited
for FFmpeg to exit while holding its full output pipe, and FFmpeg on this
device rejected `.2` as a duration. The probe now closes its read pipe before
waiting, and the duration is `0.2`. These were test-harness corrections; the
initial failed runs are retained with the final evidence.

## Device performance measurements

The same product runtime and discard sink were used with the helper explicitly
disabled/enabled. Three eligible music files were measured twice per condition:
six samples for initial play and six for seek to 30 seconds. Measurement starts
before the CLI command and ends at the first PCM read in the native sink.
Seek timing starts from an already decoding source. Jobs ran at nice 12.

| Measurement | FFmpeg | Native MP3 |
| --- | ---: | ---: |
| Initial first PCM, median | 1003.334 ms | 23.996 ms |
| Initial first PCM, observed maximum | 1054.809 ms | 34.711 ms |
| Seek to 30 s, first PCM median | 1062.595 ms | 38.197 ms |
| Seek to 30 s, observed maximum | 1124.234 ms | 52.689 ms |
| Process-tree CPU, one core basis | 8.438% | 4.712% |
| Maximum summed process-tree RSS | 61,198,336 bytes | 12,369,920 bytes |
| Maximum decoder/sink children | 2 | 2 |
| Runtime FD count before/after stop | 4 / 4 | 4 / 4 |
| Children remaining after stop | 0 | 0 |

CPU/RSS samples cover approximately 5.1 seconds of already active playback
per path on one eligible track. Summed RSS includes shared mappings and is not
PSS. These short samples exclude the GUI and real audio server; they do not
establish sustained whole-device utilization or acoustic latency. The fallback
sample retained approximately one-second first-PCM timing. No latency
percentile guarantee follows from six measurements.

The runtime fixture also passed pause/resume, seek, resume after stop, eight
rapid play/stop pairs, decoding/visualization checks and normal harness exit.
FDs and children were reclaimed while each runtime remained alive.

## PCM and regression evidence

- All 49 eligible library files were compared at their beginning and at 30 s:
  98 excerpts, each 0.2 s, against accurate FFmpeg output. All passed. Maximum
  absolute RMS error was `1.716e-7`; maximum relative RMS error on a nonzero
  reference was `1.655e-4`. Five reference excerpts were silent. This check
  took 90.098 s and its FD count remained 4 to 4. This is excerpt comparison,
  not a claim of full-file sample equivalence.
- The native ARM helper test passed VBR/CBR PCM comparison, tagged input,
  mono/48 kHz fallback, concatenated streams, absent Xing metadata, renamed
  WAV input, same-PID exec, fallback FD/argv checks, repeated launches,
  real-time pacing, SIGSTOP/SIGCONT and invalid numeric options. Synthetic
  stereo relative RMS errors were below `4.0e-7` on the device.
- Complete framebuffer suite: 110 passed. Complete Wayland suite: 108 passed.
  These do not prove acceptance of external services without credentials.
- Helper and restart/lifecycle tests passed ASan/UBSan and TSan. The latter
  used process-local ASLR disabling. No system ASLR setting was changed.
- Implementation-placement and architecture-boundary checks passed. SPDX
  headers are present in changed application/build/documentation files.
  Repository-wide SPDX remains failing on generated `AGENTS.md`, `CLAUDE.md`
  and `.claude/skills/gitnexus` instructions; those files were not altered to
  silence the check.
- GitNexus comparison against main reports 328 indexed changed symbols,
  121 affected flows and 108 tracked files, with aggregate CRITICAL risk.
  This includes the earlier adaptation work. New helper/anonymous backend
  symbols are not fully indexed; impact calls returned UNKNOWN and their
  direct lifecycle/FD paths were reviewed manually. No commit was made.

## Packages and outstanding acceptance

Both packages were built against Debian 12. The ARM64 package contains the
framebuffer runtime and APPLaunch integration; the amd64 package contains the
native Wayland runtime. Both include the private helper and a generated
`libmpg123-0 (>= 1.28.0)` dependency. Their runtime payloads remain separate.

| Candidate | SHA-256 | Main executable Build ID |
| --- | --- | --- |
| `lofibox-cardputer-zero_0.3.8-1_arm64.deb` | `c59cd8bbad21a776a903e696f3ccdbac3b28a501cc23e94ffa7dcc538e8c1da5` | `3f1ce5a696a07e850bda21c88be21d6429bdb592` |
| `lofibox_0.3.8-1_amd64.deb` | `e3bee937e298ba96137a8f1094e5313865cbe5ea9a7bc611cb0d90b5c0a76097` | `c53caa6cb3d68b29f779baa07f0ae8e2b9b11aff` |

Payload separation, ARM64 loader/version/help checks under QEMU, isolated
Debian installation/smoke/removal and installed Wayland/WebUI browser checks
passed. The browser run recorded 424 Wayland commits and no console exceptions.
Lintian reported no errors; existing changelog/manual-page/debug-directory
warnings remain. QEMU results imply compatibility, not device performance.
The device verified the ARM package SHA and apt simulation resolved replacement
with zero additional installs/removals. It was not installed on the device.

Artifacts, reports and checksums are under
`.tmp/offline-artifacts/native-mp3-20261110/`. Foreground screen/key interaction
and physical speaker acceptance of this candidate remain deferred under the
user's instruction. The earlier confirmed normal speaker output belongs to the
installed resource build. Actual LED PWM breathing remains unqualified;
this change neither modifies nor flashes the keyboard MCU.
