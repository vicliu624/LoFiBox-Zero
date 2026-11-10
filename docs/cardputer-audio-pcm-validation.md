<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# PCM contract and backlight regression validation

Date: 2026-11-10. The user reported stuttering and a screen that would not
wake during live acceptance. The preceding native MP3 and interaction packages
were withdrawn. The device was first restored to resource build
`c4a1d8a97cb21312eeaa29b815ba7c729165c789`; the user confirmed that both screen
and continuous music recovered. The corrected candidate is now installed and
the user has confirmed that playback is smooth and the screen is normal.

## Root causes and changes

The private MP3 producer emitted 44,100 Hz stereo float PCM, but the product
DSP, playback clock and sink consumed 48,000 Hz. Raw PCM has no sample-rate
header. This starved the sink, changed speed/pitch and made a 166.373-second
track end at a reported 152.736 seconds, including the 120-ms sink latency
hint. Earlier isolated tests compared against 44.1 kHz and missed the actual
product contract. They did not qualify the withdrawn packages.

`audio/pcm_format.h` now defines the shared 48 kHz stereo float32 format.
The helper retains native MP3 decoding and uses a single-threaded libsoxr HQ
resampler before output. Pacing and fallback resume positions use output
frames. EOF drains the filter tail. Seeking rebuilds 250 ms of history and
aligns the input start to the 147:160 rational conversion phase before
discarding output up to the requested 48 kHz frame. Unsupported streams still
exec the existing FFmpeg command in the same process, after releasing private
resources. No change to the product sink rate or audio quality was used to
hide the defect.

The dark-screen observation showed kernel console blanking disabled, fb0
unblanked and the actual backlight at zero while the app remained running.
The framebuffer presenter now restores a nonzero remembered brightness on
startup and keyboard input. It handles the official Fn+U/Fn+I brightness keys.
It uses the existing `video` group permission, opens no persistent descriptor,
and touches sysfs only on startup/input. It does not poll on every frame or
in the audio thread. Wayland leaves this hardware control to the compositor.

Installed-package browser testing also reproduced stale artwork after a
player restart: an empty incremental state can omit a track ID, leaving the
previous ID after a merge. The WebUI now treats `status=empty` as authoritative
when selecting artwork, including before a full snapshot arrives.

## Reproducible checks

- New `audio_pcm_contract_smoke` sends real decoder output through the actual
  product backend/DSP/clock to a PCM-capturing sink. It checks 48 kHz, stereo,
  sample count, pitch and the backend position for native and FFmpeg decoding.
  It rejects the withdrawn helper with `PCM duration differs from source at
  the actual sink rate`. On Cardputer, both corrected routes output 132,000
  frames for 2.75 seconds, with a 2.63-second reported position after latency.
- `native_mp3_helper_smoke` now uses the actual 48 kHz output contract. It
  additionally checks a 7.933-second wide-band source and arbitrary seeks
  against sliced full-history FFmpeg/libsoxr PCM, including the filter tail.
  Native wide-band relative RMS errors were below 0.000001. Existing fallback,
  pause/SIGSTOP, invalid offsets and descriptor-boundary checks remain.
- Cardputer full decode of *Lonely Street*: native and FFmpeg both output
  63,887,360 bytes = 7,985,920 stereo frames = 166.373333 seconds at 48 kHz.
  Descriptor count returned from 4 to 4.
- Cardputer library PCM comparison: 49 native-compatible songs, two excerpts
  each (start and 30 seconds), 98 comparisons passed. This is excerpt coverage,
  separate from whole-song and actual speaker acceptance. Descriptor count
  remained 4 to 4.
- Backlight test uses actual keyboard evdev injection and actual sysfs
  brightness: 50 -> 40 -> 50, then 0 -> 50 on input. Measured event-to-readback
  latencies were 33.13, 35.44 and 22.76 ms. These are software-injected events,
  not a measurement of the physical switch or optical panel response.
- Framebuffer CTest: 114/114; native Wayland CTest: 111/111. Four focused
  AddressSanitizer/UndefinedBehaviorSanitizer cases passed (PCM backend contract,
  native helper, backlight, evdev). Scoped SPDX, placement and architecture
  checks passed. Existing generated GitNexus instruction files are excluded
  from the scoped SPDX claim.
- Installed Debian package: real Weston/Chromium checks passed, including
  empty incremental artwork, restart/reconnect, two tabs, keyboard/touch,
  large-library browsing, EQ and lyrics. Both packages declare libsoxr0.
  Lintian reports no errors; existing changelog weekday, manpage and debug
  naming warnings remain.

## Candidate identity and live acceptance

Artifacts are under `.tmp/offline-artifacts/pcm48-20261110-r2/`:

| Profile | Package SHA-256 | Main ELF build ID |
| --- | --- | --- |
| Cardputer ARM64 framebuffer | `32333cb14530f09c3e6aaa9afef0cd6235b0640ecc94347cc4c89b04895272b9` | `5babe5be9adc2e79a432bb21a53e3aaad54ae1f9` |
| Debian amd64 native Wayland | `2264cce9b876fa91e488c65497976ae9ec78d7b63674c7763e7ab30d3fec0ddc` | `35bbaaf7b582fcd7410c61c5a73b96e4a601b9f9` |

Cardputer installation completed successfully and `dpkg -V` was clean.
Startup was deliberately tested with brightness zero. The app restored 50
without sudo. The system speaker volume was preserved at 45%. The first CLI
start was issued before the runtime socket existed and was correctly rejected;
the subsequent start after readiness was applied. The live sampler includes
this initial idle interval, so it must not be treated as all-playing data.

The 420-second live sampler completed with no playback errors. It includes
19 seconds before playback, then the complete first song and most of the
second song. The first song reached a sampled 166.021 seconds before advancing
from track 54 to 62. A subsequent observation, before any transport command,
confirmed that the second song also ended naturally and advanced to track 40.
The first song's sampled playback-clock/wall-clock slope was 1.00019.

The resource sampler recorded 389 samples over 420 seconds. App plus observed
descendants averaged 40.63% of one core (four cores = approximately 10.2% of
total CPU capacity), including startup/enrichment. The maximum was 125.31% of
one core. Stable samples after the playback page opened, with just the app,
decoder and sink present, had a median summed RSS of 22.11 MiB. The full-run
RSS peak was 76.25 MiB and included a transient FFmpeg artwork/enrichment
process. RSS counts shared pages per process. This is a bounded device sample,
not a claim about all codecs, network servers or memory growth over days.

Pause held the playback clock; resume, seek to 30 seconds, next, previous,
stop and restart passed through the installed CLI and shared runtime. Commands
returned in 19.87–65.19 ms across two control rounds, including CLI process
startup; these are not physical-button or speaker-onset latency measurements.
Stopping left no audio children. In the instrumented repeat, temporary
non-audio activity kept nine descriptors briefly; the count returned to the
eight-descriptor idle baseline after 904.3 ms. The earlier assertion at 150 ms
was premature and its failed run is retained alongside the completed repeat.
During the 420-second sampler the count ranged from 8 to 11, without continued
growth. Ten installed TUI views rendered successfully; CLI and WebUI agreed on
the active song, queue position and all 63 queued items, with descriptor count
10 -> 10 across that surface check.

The final player remains running and playing *Lonely Street*, with brightness
50% and system volume 45%. The user confirmed smooth sound and a normal screen
for this corrected build. MCU firmware is unchanged and experimental rhythm
lights remain disabled. Docker Desktop was unavailable for its wrapper script;
package build, dependency checks, installed smoke/browser tests and uninstall
validation instead ran in the isolated Debian 12 WSL build root.
