<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Cardputer Store submission materials

The listing is defined by the repository-root `app-builder.json`. It describes
the framebuffer package `lofibox-cardputer-zero`; the separate `lofibox` Debian
Wayland package must not be uploaded to this listing. Use the actual package
control version and architecture, then hash the exact file tested on hardware.

The existing 180×180 LoFiBox PNG is the Store icon. APPLaunch separately uses
the 64×64 icon inside the Debian package. Screenshot paths in the manifest
must be populated with actual 320×170 device captures:

- `screenshots/now-playing.png`: original demo music, readable song information,
  background spectrum and the contextual icon action bar.
- `screenshots/library.png`: a library containing only the original demo tracks.
- `screenshots/timers.png`: the real sleep and idle-screen timer settings.

These are actual Cardputer Zero framebuffer captures from the installed
`0.3.8-4` package, using a separate XDG profile and only the two original demo
tracks. Do not substitute host browser screenshots or older device captures
when updating this listing. Regenerate captures after a materially changed
candidate is installed and verified.

The listing declares network access for remote sources, metadata and WebUI.
It requests no camera, microphone, IMU, external hardware or external display.
`background_service` is false: the runtime and WebUI stop when LoFiBox exits;
there is no boot-time or persistent background service. Playback while the
display is idle remains inside the currently running application.

## Original demonstration music

On the development host, run:

```sh
python3 packaging/store/create_demo_music.py --output /path/to/store-demo
```

This requires FFmpeg and Python Mutagen, produces two one-minute 44.1 kHz
stereo MP3s with matching LRC sidecars plus `provenance.json`, and opens no audio device. Both tracks have
original synthesized audio, project-logo artwork and an embedded SYNCEDLYRICS
tag for the timed lyrics page, plus an LRC copy for review. The player uses the
embedded tag, so the MP3s are self-contained. The generator uses no files from the user's music library.
Keep generated output outside the source tree. Configure a separate XDG profile
with only this demo directory as a local root for public recording, then return
to the normal profile afterward. The initialized profile's root list must contain
only that directory; a default personal Music root must not appear in captures.

When recording a long demo, enable repeat-all explicitly in the live demo
runtime. The default all-songs view sorts the two titles alphabetically, so
Night Circuit is the last item; repeat-all makes Next wrap to Morning Light.

## Release evidence

Before submission, verify the exact ARM64 package through both an upgrade and
a fresh isolated profile, then open it from the real APPLaunch icon. Record
screen, input and speaker results; test repeated sleep/wake, wake-key repeats,
timer cancellation, playback during blanking, timed exit, launcher handoff,
resource/descriptor recovery and reopening. Keep the existing accepted music
and light behavior. Any package rebuild invalidates the tested file hash until
its payload and required checks are verified again.

The official first-submission process also requires a real-device video in the
Store PR comments. The video should show APPLaunch opening LoFiBox, browsing
the original demo library, playback and pause/skip, lyrics/queue, timer controls,
and normal exit back to APPLaunch. Show the timer/wake tests as a separate clip
if needed. Use only project-owned assets and generated demo music. Keep any
unrelated applications, personal library or account details out of captures.

Publish the matching GPL source and both platform artifacts with their hashes,
then run the official `czdev publish --deb <verified-arm64-package>` from the
repository root. Both submission routes open a PR in `CardputerZero/packages`;
new applications require maintainer review. Follow the PR's actual CI state,
and report submission and review separately from being available in the Store.

References: [official submission process](https://cardputer.cc/#/documents/cp0-submit),
[official development rules](https://cardputer.cc/#/documents/cp0-dev), and
[AppBuilder](https://github.com/CardputerZero/AppBuilder).
