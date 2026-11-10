<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Now Playing background spectrum and regular keyboard lights

Date: 2026-11-10. Scope: render the existing spectrum as a translucent Now
Playing background and replace independent frequency-onset flashes with a
regular shared clock. The accepted 48 kHz PCM producer/sink contract and the
event-driven backlight implementation are retained. No MCU firmware is flashed.

Acceptance update: the user confirmed the image and playback are normal, but
reported that these clock-driven lights do not follow the music. The light
algorithm in this report is historical and is superseded by the bass/body
onset implementation. The accepted visuals and PCM output remain unchanged.

## Behavior

The background spans x=10..305 and y=24..137 on the 320×170 logical canvas.
Thirty columns have softened edges and tips, roughly 10–25% core opacity, and
up to 114 px height. A fourth-root display curve gives quieter real bands
enough height to remain visible behind the page. Zero input stays empty.
Artwork, metadata and progress are drawn afterward; top and action bars stay
outside the spectrum. Paused, empty and unavailable states draw no spectrum.
Colors are blended against the uniform panel once per scanline. There is no
additional overlay canvas, FFT, blur, frame allocation or system I/O.

Three white keyboard indicators use one clock, starting at 100 BPM. Every
fourth pulse is 200 ms; the other pulses are 100 ms. A fixed six-interval
history accepts only consistent onset intervals, in the 67–150 BPM range;
period updates are limited to 10 ms per beat. The clock estimates tempo and
does not identify musical downbeats. Sparse or uncertain onsets retain a
stable cadence. Pause/stop/unavailable input resets the pattern immediately;
250 ms of silence suppresses it and 1.2 s resets the estimator. Stalled worker
ticks never cause catch-up bursts.

The existing latest-frame worker and keyboard-priority hardware adapter are
retained. Only BD/BE/BF off/steady mode writes are used, at a maximum 20 Hz
worker cadence, with unchanged-state writes suppressed. The adapter owns two
CLOEXEC descriptors (internal keyboard and I²C), retained for its lifetime.
APPLaunch enables the regular pattern by default; `LOFIBOX_RHYTHM_LIGHTS=0`
disables it. The Wayland target does not use this hardware adapter.

## Automated evidence

- Framebuffer suite: 114/114; Wayland suite: 111/111, before the final display
  curve adjustment. After that adjustment, focused spectrum/artwork/page
  checks and ASan/UBSan checks passed again.
- Native ARM64/QEMU checks passed for both the regular pattern/worker and the
  full-height background. These checks establish function, not performance.
- Pattern tests cover synchronous outputs, fixed pulse widths and four-beat
  grouping, gradual 120 BPM convergence, irregular input without extra pulses,
  silence, pause, unavailable/invalid values, stalled time and bounded worker
  publication with final off.
- Background tests verify tall quiet-band response, color/opacity bounds,
  untouched solid foreground pixels, no chrome intrusion, pause, no-track,
  silence and invalid data.
- Both packages passed payload/profile, dependency, installed Debian smoke,
  installed Wayland/browser and uninstall checks. The final browser run made
  417 Wayland commits with no console exceptions. Lintian reported no errors;
  pre-existing changelog weekday, DWZ naming and missing man-page warnings
  remain.

## Performance and on-device evidence

The render harness draws a representative complete Now Playing page across
1,200 measured varying-spectrum frames after 200 warm-up frames, without
framebuffer transfer or sleep. On the actual Cardputer, the previous accepted
renderer's median/p95 were 7.825/8.020 ms; the final renderer measured
8.728/9.354 ms. The median increase is 0.903 ms. This is a rendering benchmark,
not end-to-end input latency or an audio-continuity guarantee.

On the final package, a 40-second read-only 20 Hz I²C observer recorded 66
light pulses, with median interval 0.600 s and observed range 0.550–0.650 s
(50 ms observer resolution and worker scheduling). Widths repeated
0.200/0.100/0.100/0.100 s. One sample caught the sequential three-register write
mid-batch; steady samples were all-off or all-on. This does not prove literal
simultaneous electrical edges. The observer passed the playback-active,
brightness-50 and bounded-FD checks.

A 10-second PipeWire window showed the player output node at ERR=0; the ALSA
device's existing cumulative ERR=152 did not increase in that window. This
does not establish that the device has never had an xrun. The actual output
formats remained stereo F32LE/48 kHz at the client and S32LE/48 kHz at ALSA.
Source hashes for the native MP3 helper, PCM format, audio backend and
framebuffer backlight match the previously accepted PCM package.

The final package (PID 233087, ELF build ID checked after installation) passed
a 240-second uninterrupted playback observation. Track 54, Lonely Street,
ended naturally and advanced to track 62, Loving Strangers, at about 165 s of
the observation, which began roughly two seconds after playback. No sampled
audio-inactive state or playback error was observed. Backlight stayed at 50;
the process retained 12–13 descriptors during playback.

The accompanying 222 process-tree resource samples measured mean CPU 41.93%
of one core (about 10.5% of four cores), maximum 115.24% of one core, and peak
tree RSS 83,550,208 bytes (79.68 MiB); median tree RSS was 24.74 MiB.
The tree includes transient artwork/metadata
subprocesses. Short-lived children between samples may be missed; this is not
a byte-accurate peak-memory guarantee. This observation is not a controlled
same-track CPU comparison against the older build.

Three final control cycles passed play/pause/resume, seek, next/previous and
stop. CLI command completion was 18.83–65.73 ms. Pause held the playback clock;
pause and stop read back all three modes as off. Every cycle returned to the
same ten-descriptor stopped baseline, including the two retained light-adapter
descriptors. Auxiliary metadata jobs can still open and close pipes after stop;
the longest observed final return to baseline was 7.74 s. The observations
identify ffprobe/curl and related enrichment jobs rather than accumulating
light-controller handles. This is not a claim that stop instantly cancels all
metadata work.

Software evdev injection also verified brightness down/up (11.77/25.28 ms) and
wake from zero (33.74 ms), ending at brightness 50. This is not physical key
latency. The player was left on Now Playing with music active and system
volume 0.45. Physical appearance and audible continuity require user
observation in addition to process data; the user has not yet confirmed this
visual build. Raw final-device reports and observation scripts are in
`device-evidence.zip` beside the packages.

The first visual candidate was refined after its actual framebuffer capture
showed quiet music still concentrated near the bottom. Its initial observer
also inherited brightness 100 from the intervening APPLaunch session, so that
run does not pass the brightness-50 assertion. Brightness was restored before
the final candidate. An initial rapid-control check observed an auxiliary
descriptor beyond its four-second window; subsequent checks recorded its
targets and eventual return to the ten-descriptor stopped baseline. These
intermediate runs do not qualify the final candidate's soak.

## Candidate artifacts

The final candidates are in `.tmp/offline-artifacts/dynamic-20261110-r2/`:

- Cardputer framebuffer ARM64: `lofibox-cardputer-zero_0.3.8-1_arm64.deb`,
  SHA-256 `eaa601681fed0e31ea2f8bcae3b9d0cbdab1d94774c900e9daf6a392209a2608`,
  ELF Build ID `05097970c9e8dc255ce1fdaca21442a5a99c38c6`.
- Debian Wayland amd64: `lofibox_0.3.8-1_amd64.deb`,
  SHA-256 `b9d0029b2e984e2c2fa58349c1f7853f998d6a4ba82f2d146e0dfa61ad0a343c`,
  ELF Build ID `171b111e240fcee34c0e782db57d2fe9e9bd8b02`.

The earlier `pcm48-20261110-r2` package remains the rollback point with user
confirmation of smooth audio and normal screen operation. Firmware breathing
experiments and the first `dynamic-20261110` package are not final deliverables.
