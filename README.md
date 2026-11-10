<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero

<p align="center">
  <img src="docs/assets/logo.png" alt="LoFiBox Zero logo" width="120">
</p>

LoFiBox Zero is a Linux-first music player for compact keyboard-driven devices and normal compositor-managed desktop sessions. It keeps one focused player canvas across the Cardputer Zero class of hardware, the Linux framebuffer device target, and native Linux Wayland/X11 desktop targets.

It is built around local library playback, durable media roots, metadata and artwork enrichment, lyrics, queue control, remote media sources, and a DSP-ready audio pipeline. The goal is a small music appliance that feels deliberate rather than a resized desktop app.

## Player Surface

| Now Playing | Library Menus |
|:--:|:--:|
| <img src="docs/assets/playing.png" alt="LoFiBox Zero now-playing page with album art, transport state, progress, and spectrum visualization" width="420"> | <img src="docs/assets/menus.png" alt="LoFiBox Zero menu and library navigation pages on the compact player surface" width="420"> |
| Lyrics | Equalizer |
| <img src="docs/assets/lyric.png" alt="LoFiBox Zero lyrics page with timed lyric focus and playback context" width="420"> | <img src="docs/assets/EQ.png" alt="LoFiBox Zero equalizer page with band controls and spectrum visualization" width="420"> |

The screenshots show the product surface, not a mock desktop simulator. LoFiBox keeps one runtime path and lets the target adapters provide Linux presentation, input, audio, filesystem, and service integration.

## Highlights

- Compact player UI tuned for small screens and desktop windows
- Local library indexing from durable configured media roots, defaulting to `~/Music`
- Queue, search, lyrics, artwork, metadata enrichment, and tag writeback services
- Remote media profiles for Jellyfin, Emby, OpenSubsonic/Navidrome-style servers, playlists, WebDAV, DLNA/UPnP, and other governed source families
- Runtime command surface for live playback, queue, EQ, diagnostics, and library refresh
- Direct CLI, TUI, Linux Wayland, Linux X11, and Linux framebuffer/device targets sharing the same app semantics
- Debian-oriented build, packaging, AppStream, desktop integration, and release discipline

The repository intentionally keeps one product runtime path:

- shared product code in `src/app` and `src/core`
- Linux runtime adapters in `src/platform/host`, `src/platform/wayland`, `src/platform/x11`, and `src/platform/device`
- a single Linux device executable: `lofibox_zero_device`
- a protocol-selecting launcher command: `lofibox`
- a native Linux Wayland desktop-application executable: `lofibox_zero_wayland` (`lofibox-wayland`)
- a direct Linux X11 desktop-application executable: `lofibox_zero_x11` (`lofibox-x11`)
- a containerized Linux build environment for repeatable builds

There is no SDL desktop simulator in this project. The app should be validated through the Linux device target, a real framebuffer/input environment, or a container wired to those Linux devices.

## Repository Layout

- `src/app`: product state, pages, controllers, playback/library semantics
- `src/core`: canvas, font, display primitives, platform-neutral utilities
- `src/platform/host`: host services such as metadata, artwork, lyrics, audio process launch, logging, caching, and single-instance lock
- `src/platform/wayland`: native Wayland presentation adapter for normal compositor-managed desktop windows
- `src/platform/x11`: X11 presentation adapter for traditional desktop sessions
- `src/platform/device`: Linux framebuffer and evdev/xkb input adapters
- `src/targets/device_main.cpp`: Linux product entry point
- `assets`: icons, logo, fonts, and other product assets
- `docs/assets`: README and documentation screenshots
- `docker/dev-container.Dockerfile`: container image for Linux builds
- `scripts/run-dev-container.*`: build/run helper for the Linux device target inside the container

## Build In Container

Release builds have two product profiles: **Cardputer Zero uses framebuffer**
(`lofibox-cardputer-zero` package), and **Debian desktop uses Wayland**
(`lofibox` package). Use `cardputer-release` / `cardputer-release-build` or
`debian-wayland-release` / `debian-wayland-release-build` CMake presets.
See [packaging](docs/packaging.md#product-artifacts) for the matching package build
profiles and [offline validation](docs/cardputer-offline-validation.md) for the
performance work and the remaining device checks.

Build the development image:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build-dev-container.ps1
```

Build and run the Linux device target from Windows through WSL:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run-dev-container.ps1 -MediaRoot C:\Users\VicLi\Music
```

Open an interactive shell instead:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run-dev-container.ps1 -Mode shell -MediaRoot C:\Users\VicLi\Music
```

The device executable expects Linux device paths. Override them with:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run-dev-container.ps1 -MediaRoot C:\Users\VicLi\Music -- --fbdev /dev/fb0 --input-dev /dev/input/event0
```

Equivalent environment variables:

- `LOFIBOX_FBDEV`
- `LOFIBOX_INPUT_DEV`
- `LOFIBOX_MEDIA_ROOT`
- `LOFIBOX_RUNTIME_LOG_PATH`
- `XDG_STATE_HOME`
- `XDG_CACHE_HOME`

## Build Locally On Linux

```bash
cmake -S . -B build/device -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DLOFIBOX_BUILD_DEVICE=ON \
  -DBUILD_TESTING=OFF

cmake --build build/device --target lofibox_zero_device
```

Run:

```bash
LOFIBOX_MEDIA_ROOT="$HOME/Music" \
./build/device/lofibox_zero_device --fbdev /dev/fb0 --input-dev /dev/input/event0
```

Build the native Linux Wayland desktop-application target:

```bash
cmake --preset linux-wayland-debug
cmake --build --preset linux-wayland-debug-build
```

Run inside a Wayland session:

```bash
LOFIBOX_MEDIA_ROOT="$HOME/Music" \
./build/wayland/lofibox-wayland
```

Enable the browser WebUI for direct development runs:

```bash
LOFIBOX_WEBUI=1 LOFIBOX_WEBUI_BIND=0.0.0.0 LOFIBOX_WEBUI_PORT=8765 \
./build/wayland/lofibox-wayland
```

The Linux WebUI code is compiled by default on Linux builds. It is intentionally
not started by the generic `lofibox`, `lofibox-wayland`, or `lofibox-x11`
commands unless `--webui` or `LOFIBOX_WEBUI=1` is supplied. The Cardputer Zero
APPLaunch wrapper is the appliance launch profile, so it enables WebUI by
default and Settings shows the resulting address as `http://<device-ip>:8765`
unless a caller overrides the bind address or port.

Build the direct Linux X11 desktop-application target:

```bash
cmake --preset linux-x11-debug
cmake --build --preset linux-x11-debug-build
```

Run:

```bash
LOFIBOX_MEDIA_ROOT="$HOME/Music" \
./build/x11/lofibox-x11
```

This is a real Linux presentation target using the same app, playback, library, remote-source, metadata, DSP, persistence, and credential semantics as the device target.

## Preview APT Repository

Before LoFiBox is available from the official Debian archive, preview packages
can be published as a signed GitHub Pages APT repository.

```bash
sudo install -d -m 0755 /etc/apt/keyrings

curl -fsSL https://vicliu624.github.io/lofibox-apt/lofibox-archive-keyring.pgp \
  | sudo tee /etc/apt/keyrings/lofibox-archive-keyring.pgp >/dev/null

sudo chmod 0644 /etc/apt/keyrings/lofibox-archive-keyring.pgp

sudo tee /etc/apt/sources.list.d/lofibox.sources >/dev/null <<'EOF'
Types: deb
URIs: https://vicliu624.github.io/lofibox-apt/debian
Suites: trixie
Components: main
Signed-By: /etc/apt/keyrings/lofibox-archive-keyring.pgp
EOF

sudo apt update
sudo apt install lofibox
```

The preview repository uses a per-repository keyring and `Signed-By`; do not use
`apt-key`. The official Debian archive remains the long-term target.

## Optional Host Fingerprinting

The development container already installs `libchromaprint-tools`. For direct host runs, use:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\ensure-host-fpcalc.ps1
```

To let the script install Chromaprint/fpcalc through a supported package manager:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\ensure-host-fpcalc.ps1 -Install
```

The runtime also honors `FPCALC_PATH` when `fpcalc` is installed outside `PATH`.

## Tests

Shared tests do not require the Linux device adapter:

```powershell
cmake --preset windows-vs2022-test-debug
cmake --build --preset windows-vs2022-debug-build-tests
ctest --test-dir build\test -C Debug --output-on-failure
```

Linux validation:

```bash
bash scripts/wsl-validate.sh
```

## Cardputer Keyboard Controls

The framebuffer GUI follows the device's [keyboard specification](https://cardputer.cc/docs/zh-CN/CP0_keys.md).
The keyboard driver already translates Fn combinations into Linux keycodes;
LoFiBox does not emulate the firmware's sticky Fn/Shift/Sym state.

| Keys | Action |
| --- | --- |
| F / Z / X / C | Up / left / down / right outside text editors |
| Enter | Open, confirm or play the selected item |
| Short ESC | Back; cancels an unsaved field edit |
| Hold ESC for 2 seconds | Exit the framebuffer app and return control to APPLaunch |
| Fn+H or F1 | Toggle page help |
| Fn+Q / Fn+W / Fn+E | Play/pause / previous track / next track, including while help or an editor is open |
| Fn+S / Fn+D / Fn+A | System volume down / up / mute in the framebuffer app |
| 4-8 | The five actions displayed along the bottom of the current page |
| Fn+L / Fn+M | Page up / page down; larger EQ adjustments on the equalizer page |
| Fn+K | Player main menu |

Search and remote-profile fields accept literal letters, spaces and digits,
including F/Z/X/C and 4-8. Backspace deletes text; Escape cancels/leaves the
editor. Held navigation/typing keys may repeat; held help, transport and numeric
screen actions do not repeat. Existing F2-F6 and F9-F12
bindings remain available. System volume changes use one coalescing background
worker so PipeWire/PulseAudio helper calls never block input or rendering.

The five bottom actions line up with the physical 4-8 keys beneath the display
at x=49/105/161/217/273. A consistent set of crisp icons replaces the stacked
action labels and digits on every page. Primary actions use the theme accent;
play/pause updates with playback state. The top-left help control is also an
icon. The same page definition supplies keyboard actions, pointer actions and
the full descriptions in F1/FnH help. Search and field editors show keyboard,
confirm and cancel icons while preserving literal numeric input.

| Page | 4 | 5 | 6 | 7 | 8 |
| --- | --- | --- | --- | --- | --- |
| Main menu | Left item | Open | Right item | Player | Search |
| Now playing | Previous | Play/pause | Next | Lyrics | Queue |
| Lyrics / queue status | Previous | Play/pause | Next | Player | Play mode |
| Music categories / remote browse | Back | Page up | Open | Page down | Search |
| Songs / playlist tracks | Back | Page up | Play selection | Page down | Sort |
| Playlists | Back | Page up | Open | Page down | Search |
| Equalizer | Previous band | Gain -1dB | Next preset | Gain +1dB | Next band |
| Settings / remote configuration | Back | Page up | Open/change | Page down | Help |
| Server diagnostics | Back | Page up | Help | Page down | Settings |
| Stream details | Back | — | Play stream | — | Help |
| Playlist editor (unfinished) | Back | Page up | Help | Page down | Search |
| About | Back | Library | Player | Settings | Help |
| Search / field editor | Literal 4 | Literal 5 | Literal 6 | Literal 7 | Literal 8 |

List content reserves five visible rows above the action bar, and its scrolling
and page movement use that same row count. A help modal consumes numeric screen
actions until closed, while dedicated Fn media controls remain available.
The playlist editor's existing create/remove/reorder/save rows are not wired to
commands yet; the action bar does not advertise those operations.

## Cardputer Rhythm Lights

The three keyboard indicators follow the rightmost Now Playing spectrum column.
The renderer and lights share the same band-to-height conversion. A rise of
four pixels from a trough turns the lamps on; a fall of four pixels from a crest
turns them off. Smaller reversals are ignored, and heights below ten pixels are
dark. A held crest can hold the lamps on. There is no beat estimate or pulse
timer: elapsed time or changes in the other nine bands cannot create flashes.
They are **white single-color LEDs**, as shown
on page 8 of the [official schematic](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1243/C154-CardputerZERO_SCH_V0.6.1_20260702.pdf#page=8);
the built-in hardware cannot produce RGB colors. Silence, pause, stop and normal
exit turn the music indications off.
Shift/Fn/Sym indications take priority over the music pattern.

The Cardputer APPLaunch wrapper enables the spectrum-following lights by default;
set `LOFIBOX_RHYTHM_LIGHTS=0` to disable it. The stock LED-mode interface is used;
smooth breathing is **not implemented** and requires no firmware experiment
to use this pattern.
The Debian Wayland runtime does not access the
Cardputer light controller. The device adapter validates the internal keyboard
and device-tree mapping before accessing the three LED-mode registers; it
does not change the backlight, PWM frequency or GPIO power controls.

Now Playing renders its spectrum behind the artwork and metadata across the
page body, with soft column tips and roughly 10–25% opacity. The top bar and
action bar remain clear. This layer reuses the existing visualization and blends
once per scanline without an extra canvas, blur pass or audio analysis.

The lights reuse the player's existing ten-band visualization. One background
worker samples the latest frame at up to 20 Hz, without another decoder or a
queue of stale animation frames. Input and rendering never perform the light
controller's I2C operations. The experimental brightness probes are validation
scripts only and are not part of the installed player.

## Sleep And Screen Timers

In Settings, select **SLEEP TIMER** and press Enter or the open/change icon
(Cardputer key **6**) to cycle OFF, 15, 30, 45, 60, 90 and 120 minutes.
At expiry, LoFiBox stops playback and exits to its launcher. It does not power
off the device. Select OFF to cancel, or another duration to restart the countdown.

The separate **SCREEN OFF** setting on the framebuffer build cycles OFF,
30 seconds, 1, 2, 5 and 10 minutes of keyboard inactivity. Music continues while
the backlight is off; drawing and framebuffer copies stop, and the music LEDs
go dark. Any physical key wakes the display at its previous brightness. That
wake press, including held repeats, is consumed before playback/navigation
commands resume. Changing screen timeout does not reset the exit countdown.
Wayland delegates monitor power management to the desktop environment and
does not offer the device backlight timer.

Timers are session settings and start OFF each time LoFiBox opens. CLI users
can use `lofibox-cardputer-zero runtime settings apply '' '' 30M 1M` (on Debian,
use `lofibox` and omit the framebuffer-only final argument). Empty arguments
leave settings alone; OFF cancels a timer. TUI's command palette accepts
`sleep 30m`, `sleep off`, `screen 1m` and `screen off`. WebUI's More tab exposes
the same shared runtime settings. CLI/API durations accept positive integer
S/M/H values up to 24 hours; invalid values leave the entire update unchanged.

## Runtime Boundary

The product boundary is:

```text
Source -> Decode/Playback Services -> DSP/Visualization -> App State -> Canvas -> Linux Presenter
```

The app layer must not know whether it is running in a container, on a Cardputer Zero, or on another Linux device. Those facts belong to platform adapters and startup configuration.

The Linux input truth is the kernel input device stream. Product behavior should be asserted against logical app input events, not against screenshots, product photos, or mock shell geometry.
