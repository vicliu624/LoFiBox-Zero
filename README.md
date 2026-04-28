<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero

LoFiBox Zero is a Linux-first music player for the Cardputer Zero class of devices.

The repository intentionally keeps one product runtime path:

- shared product code in `src/app` and `src/core`
- Linux runtime adapters in `src/platform/host` and `src/platform/device`
- a single Linux device executable: `lofibox_zero_device`
- a direct Linux X11 desktop-widget executable: `lofibox_zero_x11`
- a containerized Linux build environment for repeatable builds

There is no SDL desktop simulator in this project. The app should be validated through the Linux device target, a real framebuffer/input environment, or a container wired to those Linux devices.

## Repository Layout

- `src/app`: product state, pages, controllers, playback/library semantics
- `src/core`: canvas, font, display primitives, platform-neutral utilities
- `src/platform/host`: host services such as metadata, artwork, lyrics, audio process launch, logging, caching, and single-instance lock
- `src/platform/device`: Linux framebuffer and evdev/xkb input adapters
- `src/targets/device_main.cpp`: Linux product entry point
- `assets`: icons, logo, fonts, and other product assets
- `docker/dev-container.Dockerfile`: container image for Linux builds
- `scripts/run-dev-container.*`: build/run helper for the Linux device target inside the container

## Build In Container

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

Build the direct Linux X11 desktop-widget target:

```bash
cmake --preset linux-x11-debug
cmake --build --preset linux-x11-debug-build
```

Run:

```bash
LOFIBOX_MEDIA_ROOT="$HOME/Music" \
./build/x11/lofibox
```

This is a real Linux presentation target using the same app, playback, library, remote-source, metadata, DSP, persistence, and credential semantics as the device target.

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

## Runtime Boundary

The product boundary is:

```text
Source -> Decode/Playback Services -> DSP/Visualization -> App State -> Canvas -> Linux Presenter
```

The app layer must not know whether it is running in a container, on a Cardputer Zero, or on another Linux device. Those facts belong to platform adapters and startup configuration.

The Linux input truth is the kernel input device stream. Product behavior should be asserted against logical app input events, not against screenshots, product photos, or mock shell geometry.
