<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Music-driven keyboard light pulses

Date: 2026-11-10. The user confirmed the previous candidate's image and
playback were normal, but reported that its lights looked like a fixed-speed
clock rather than following the music. The earlier four-beat clock is replaced.

Superseded: the user also found this bass/body onset effect insufficiently
related to the music and requested direct synchronization with the rightmost
Now Playing column. This report is historical; see the spectrum-following
implementation for the current behavior.

## Change and boundaries

Every light pulse now requires an observed bass/body attack in the existing
ten-band visualization. Bass weights the 63/125/250 Hz bands; body weights
500/1000/2000 Hz. Positive flux and contrast against a 750 ms level envelope
use adaptive thresholds with small noise floors. Release hysteresis prevents
a continuing rise from retriggering; a 280 ms refractory interval limits extra
pulses. All three indicators share each 100 ms pulse. There is no default
tempo, predicted pulse or invented four-beat accent. Sustained tones and
missing beats do not cause a continuing flash clock.

Pause, unavailable input and a worker gap over 250 ms reset the detector.
Silence suppresses output within 150 ms. Initializing with a steady nonzero
frame does not itself count as an onset. Keyboard indications still have
priority. The unchanged 20 Hz worker owns its existing two CLOEXEC descriptors
and performs all hardware I/O outside the rendering/audio threads.

Only the device pattern header/implementation and its test changed among
`src/`, `data/` and `tests/` versus the accepted visual candidate. In particular,
the PCM backend/helper/format, Now Playing renderer, framebuffer/backlight,
input handling and light-controller register-access implementation are
unchanged. The Debian Wayland ELF Build ID is unchanged, demonstrating that
the Cardputer-only detector is outside the desktop runtime.

## Offline checks

Focused light, backlight, spectrum and launcher checks passed. ASan/UBSan and
native ARM64/QEMU light/background checks passed. Tests establish exact
onset timing at arbitrary phase, tempo changes and a skipped beat at both
quiet and loud levels; fixed 100 ms pulse length; no free-running flashes on
sustained tones; no repeated crescendo pulses; rejection of tiny noise and
isolated treble changes; pause/unavailable/stall resets; bounded latest-frame
publication and final off. QEMU is functional evidence only.

Existing-runtime spectrum and LED-mode captures were made at 20 Hz while
playing 60 s of Luv(sic.)pt3 and 30 s of Lonely Street. The new production
pattern replay emitted 70 and 24 pulses, versus the old clock's 109 and 50.
This shows the changed response on real inputs, not subjective synchronization.

An independent offline PCM bass-attack proxy was also compared using local
copies of the same MP3s. Median absolute distance to a proxy attack changed
from 90.6 to 53.1 ms on Luv(sic.)pt3, and from 92.4 to 83.1 ms on Lonely Street.
The latter mean distance did not improve (109.2 to 111.2 ms). These are
unannotated signal proxies, not ground-truth beat labels or measured physical
speaker/LED timing. There is no claim of perfect beat or downbeat detection.
User observation remains necessary to judge perceptual improvement.

## Packages and live acceptance

Artifacts: `.tmp/offline-artifacts/beat-20261110/`.

- Cardputer framebuffer ARM64 package SHA-256:
  `927e6a3dc087a841e826ab2d855e352d76fc6f308abf39186e1eb2299e521ea5`;
  main ELF Build ID `bdd74a644de20693285e487d974b7ca848761047`.
- Debian Wayland amd64 package SHA-256:
  `374e173837ea2b893ca11ffed23b5eded1a937e3375b0d775490d65cb7d612ce`;
  main ELF Build ID `171b111e240fcee34c0e782db57d2fe9e9bd8b02`.

Live installation, playback, LED readback, resource and control results follow
after device acceptance. The earlier visual package remains the rollback.
