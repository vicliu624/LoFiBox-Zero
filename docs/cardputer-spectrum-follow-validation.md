<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Rightmost spectrum column and keyboard lights

Date: 2026-11-10. The user requested that the three lamps follow the rightmost
Now Playing spectrum column after finding both the clock and bass/body onset
effects insufficiently related to the music. This replaces both algorithms.

## Behavior and scope

The renderer and device adapter now use `core/spectrum_display.h` for the same
last-band-to-height conversion (0..114 px). All three lamps turn on after a
four-pixel rise from a trough, and off after a four-pixel fall from a crest.
The extremum follows the current direction. Smaller reversals retain the
state; a held crest may keep the lamps on. Heights below ten pixels are dark.
The initial observed height establishes the reference without a startup flash.

Only `bands.back()`, the visible last column, drives this state machine.
Elapsed time and the other nine bands cannot cause a transition. There is no
BPM estimate, onset mixture, default flash clock or fixed pulse duration.
Pause, unavailable input and a worker stall reset the state. Modifier priority,
the stock BD/BE/BF off/steady interface, the existing 20 Hz worker and its two
CLOEXEC descriptors are retained. No firmware, backlight or audio-path change
is involved.

The Now Playing renderer uses the shared helper in place of its identical
local expression. A deterministic complete-page rendering before and after
the extraction was byte-identical. Host render medians were 613.6/617.3 µs in
that run; these are host measurements, not Cardputer frame or input timings.

## Checks

- Pattern tests cover four-pixel rise/fall boundaries, jitter rejection,
  retained crest/trough states, isolation from the other nine bands, silence,
  pause, unavailable/invalid input, stalled ticks, bounded asynchronous frame
  publication and final off.
- Spectrum and light tests passed normally and under ASan/UBSan. Native
  ARM64/QEMU functional checks passed; this is not performance evidence.
- Both framebuffer ARM64 and native Wayland amd64 packages passed profile,
  dependency, installed browser/Wayland smoke and uninstall checks. The browser
  run recorded 429 Wayland commits with no console exceptions. Lintian had no
  errors; existing changelog weekday, DWZ and man-page warnings remain.

## Live candidate observations

The candidate's ELF Build ID is `5b84a922b496a8860d0c692335d5bf7337db87f9`,
the same as the packaged Cardputer executable. It played Luv(sic.)pt3 from
the user's library in Now Playing, at backlight 50 and system volume 0.45.

A 60-second, 20 Hz runtime/I²C observer collected 1,200 samples. The visible
last-band height ranged from 41 to 81 px. It recorded 175 rising lamp edges
and 349 total transitions; five samples caught the sequential register writes
mid-batch. All sampled audio states were active. Replaying the height rule
against the asynchronous samples agreed on 77.71% of exact sample states,
and 99.39% with a ±100 ms sampling window. This checks the data/control
relationship; it is not camera-measured LED/screen latency or subjective
acceptance, and the tolerance must not be omitted when citing the result.

The 120-second process-tree profile collected 111 samples: mean CPU 53.07%
of one core (13.27% of four cores), maximum 60.28% of one core, peak RSS
47,288,320 bytes (45.10 MiB), median RSS 23.71 MiB. During the 20 Hz observer
the mean was 56.03%; after that observer it was 50.20% over 51 later samples.
These are different musical passages and not a controlled A/B comparison.
Short-lived children between samples can be missed. A separate thread sample
identified the GUI thread as the dominant CPU consumer (42.68% of one core),
with the other sampled threads at 1.7%, 0.4% or zero.

## Artifacts

The accepted Cardputer build and companion Debian package are in
`.tmp/offline-artifacts/spectrum-follow-20261110/`:

- Cardputer framebuffer ARM64 SHA-256:
  `e49d1b4b2b08fc696fd1a2b4f781907e5e5b56411728119b1f4f882a3e5b7aaf`;
  main Build ID `5b84a922b496a8860d0c692335d5bf7337db87f9`.
- Debian Wayland amd64 SHA-256:
  `15331fa14754ed13e4d0d03c4df99a6d0b7ec644c20922183f915276f6357cb1`;
  main Build ID `8c87a69a952b16e43cb3a2435711b82082a1f6f1`.

The final Cardputer package was installed and verified with `dpkg -V`; the
installed executable's Build ID matches the candidate. The normal installed
APPLaunch entry was then started (PID 238745). Play/pause/resume/stop commands
completed in 18.56–28.41 ms, pause held the clock, and both pause and stop read
back all lamps off. Stop returned to ten descriptors, including the existing
two retained light descriptors. Software evdev injection verified brightness
down/up in 23.12/36.01 ms and wake from zero in 25.96 ms; these are not physical
key timings. The player was left in Now Playing playing Luv(sic.)pt3, backlight
50 and system volume 0.45.

On 2026-11-10 the user confirmed the visible relationship between the rightmost
spectrum column and the three lamps: “能看出同步，先用这一版”. The user had also
confirmed normal image and playback in the preceding visual acceptance. The
current spectrum-following Cardputer build is therefore retained as the
accepted version. No further light-pattern change is requested. This user
confirmation concerns the physical Cardputer; Debian validation remains the
automated evidence described above. No executable or package was changed
when recording this acceptance.

The accepted visual/PCM package remains available for rollback.
