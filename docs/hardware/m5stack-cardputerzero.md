<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# M5Stack CardputerZero

## `1` Overview

* This page records the current hardware specification baseline for `M5Stack CardputerZero`.
* `CardputerZero` is a Linux handheld built around a Raspberry Pi compute module, not an ESP32-class MCU board.
* This document is intended to serve as the implementation baseline for future `LoFiBox` porting work.

> [!IMPORTANT]
> The official `CardputerZero` product page is still in a pre-launch state as of `2026-04-23`.
> Some official specifications are marked as preliminary and may change before mass release.
> This document therefore distinguishes between:
> 
> * official hardware claims
> * model differences between `CardputerZero` and `CardputerZero Lite`
> * capabilities that are already proven by the local Linux demo code

## `2` Product Identity

| Item | Spec |
| ---- | ---- |
| Product | `M5Stack CardputerZero` |
| Product Class | Pocket Linux terminal |
| System Model | `AArch64 Linux` handheld |
| Main Module | `Raspberry Pi Compute Module 0 (CM0)` |
| SoC | `Broadcom BCM2837` |
| CPU | `Quad-core Cortex-A53 @ 1GHz` |
| RAM | `512MB LPDDR2` |

## `3` Core Hardware Features

| Feature | CardputerZero |
| ------- | ------------- |
| Display | `1.9"` LCD |
| Display Driver IC | `ST7789V3` |
| Display Resolution | `320 x 170` |
| Keyboard | `46-key matrix keyboard` |
| Wireless | `2.4GHz Wi-Fi 802.11 b/g/n`, `Bluetooth 4.2 / BLE` |
| Ethernet | `10/100M` |
| Storage | `microSD card slot` |
| Battery | `3.7V 1500mAh Li-Po` |
| Fuel Gauge | `BQ27220` |
| Audio Codec | `ES8389` |
| Audio Input | `MEMS mic` |
| Audio Output | `1W speaker`, `3.5mm TRS out` |
| RTC | `RX8130CE` |
| IR | `TX & RX` |
| Expansion | `HY2.0-4P`, `2.54-14P bus` |
| USB | `2x USB Type-C`, `1x USB-A`, `USB 2.0 Host/Slave switchable` |
| Video Output | `HDMI output`, `1080P 30fps` |

## `4` Full Model vs Lite

The official comparison currently distinguishes two models.

| Feature | CardputerZero Lite | CardputerZero |
| ------- | ------------------ | ------------- |
| Raspberry Pi CM0 | Yes | Yes |
| Wi-Fi & Bluetooth | Yes | Yes |
| IR TX & RX | Yes | Yes |
| 8MP Camera | No | Yes |
| IMU | No | Yes |
| Bundled 32GB microSD | No | Yes |

> [!NOTE]
> If the target device is `CardputerZero Lite`, camera- and IMU-related assumptions must be treated as invalid by default.

## `5` Display Model

### Hardware Spec

| Item | Spec |
| ---- | ---- |
| Panel | `1.9"` LCD |
| Driver IC | `ST7789V3` |
| Resolution | `320 x 170` |

### Linux Exposure Model

In the local `M5CardputerZero-UserDemo`, the display is not driven through a microcontroller graphics stack.
It is consumed as a Linux framebuffer device:

* The demo scans `/proc/fb`
* It looks for `fb_st7789v`
* It then opens the matching `/dev/fbX`
* LVGL uses the Linux `fbdev` backend to render

This means the `LoFiBox` display port should be built around:

* Linux framebuffer
* LVGL Linux backend
* `320 x 170` UI layout assumptions

and not around Arduino display drivers.

### Proven by Local Demo

The following are already visible in local code:

* `fb_st7789v` device detection
* `LV_LINUX_FBDEV_DEVICE` environment override
* runtime framebuffer resolution query through LVGL

## `6` Keyboard Model

### Hardware Spec

| Item | Spec |
| ---- | ---- |
| Keyboard Type | `46-key matrix keyboard` |

### Linux Exposure Model

For application development, the keyboard is treated as a Linux input device, not as a GPIO-scanned matrix.

The local demo uses:

* default input device path: `/dev/input/by-path/platform-3f804000.i2c-event`
* default keymap file: `/usr/share/keymaps/tca8418_keypad_m5stack_keymap.map`
* Linux evdev key events
* LVGL keypad events

### Input Abstraction Notes

The local `APPLaunch` project goes beyond simple directional input:

* it stores `key_code`
* it also keeps `keysym`
* it also keeps `codepoint`
* it also keeps `utf8`

This suggests the target input model is a true keyboard-capable Linux terminal input path, not just a menu navigation pad.

### Proven by Local Demo

The following are already visible in local code:

* evdev-backed keyboard path
* custom key queue for Linux input events
* mapping from Linux keycodes to `LV_KEY_*`
* event forwarding into LVGL UI objects

## `7` Audio Model

### Hardware Spec

| Item | Spec |
| ---- | ---- |
| Codec | `ES8389` |
| Microphone | `MEMS mic` |
| Speaker | `1W speaker` |
| Analog Audio Out | `3.5mm TRS out` |

### Application-Level Interpretation

Unlike the ESP32 port path, audio here should be treated as a Linux user-space audio problem.
The local Linux demo does not expose direct codec register access or raw I2S routing in the application.
Instead, it treats playback as an OS-level service.

### Proven by Local Demo

The local `APPLaunch` music page already proves that the device is expected to support Linux-side media playback:

* it scans folders for `.mp3`
* it forks a child process for playback
* it first tries `mpg123`
* it falls back to `ffplay`

This supports the following conclusion:

* audio output is already expected to work through the Linux environment
* future `LoFiBox` work can start from Linux audio playback assumptions

### Not Yet Proven by Local Demo

The following hardware exists in official product material, but is not yet proven by the local demo code:

* microphone capture path
* runtime mixer / volume control
* direct low-level codec management from the app

## `8` Storage Model

| Item | Spec |
| ---- | ---- |
| Removable Storage | `microSD card slot` |
| Full Model Bundle | `32GB microSD included` |
| Lite Model Bundle | `not included` |

For `LoFiBox`, this makes the Linux filesystem plus removable media model a natural fit for:

* local music library indexing
* playlist persistence
* cover art storage
* media scanning

## `9` Power and Telemetry

### Hardware Spec

| Item | Spec |
| ---- | ---- |
| Battery | `3.7V 1500mAh Li-Po` |
| Fuel Gauge | `BQ27220` |

### Current Software Status

The local demo UI shows battery and charging information in some places, but those values are currently presentation placeholders rather than confirmed live telemetry.

That means:

* battery hardware should be considered present
* live battery integration should be considered pending unless separately proven through Linux system interfaces

## `10` Connectivity and Expansion

| Item | Spec |
| ---- | ---- |
| Wi-Fi | `2.4GHz 802.11 b/g/n` |
| Bluetooth | `4.2 / BLE` |
| Ethernet | `10/100M` |
| Grove Port | `HY2.0-4P` |
| Expansion Bus | `2.54-14P` |
| Bus Types Mentioned Officially | `SPI`, `I2C`, `UART`, `USB`, `GPIO`, `5V` |
| IR | `TX & RX` |

This means `CardputerZero` is not just a fixed playback terminal.
It should be treated as a Linux handheld with meaningful external expansion potential.

## `11` Optional Sensors and Media Hardware

The full `CardputerZero` model currently includes the following optional-on-full-model hardware:

| Item | Full Model | Lite |
| ---- | ---------- | ---- |
| Camera | `Sony IMX219 8MP` | No |
| IMU | `BMI270` | No |
| RTC | `RX8130CE` | Not called out as removed |

> [!NOTE]
> For porting work, camera and IMU should be modeled as optional capabilities, not unconditional requirements.

## `12` Linux Demo Reality Check

The local Linux demo code is useful, but it mixes real hardware integration with UI placeholders.

### Proven by Local Demo

* `AArch64 Linux` target model
* `ST7789V` framebuffer display path
* `320 x 170` UI target size
* Linux evdev keyboard input
* keymap-backed keyboard interpretation
* Linux process-based MP3 playback path

### Present in UI but Not Yet Proven as Live System Integration

* battery percentage
* charging status
* Wi-Fi SSID
* IP address
* sound volume state
* display brightness state

These should not be treated as implemented hardware service integrations until real Linux data sources are wired in.

## `13` Porting Implications for LoFiBox

This hardware profile implies that a `CardputerZero` port should be approached as a Linux application port, not an MCU board bring-up.

### Recommended Baseline

* Display: `LVGL + Linux framebuffer`
* Input: `evdev keyboard`
* Audio output: Linux user-space playback stack
* Storage: Linux filesystem + microSD
* Optional later work: battery telemetry, volume integration, microphone capture

### Do Not Assume

* Arduino display stack
* MCU-style GPIO keyboard scanning
* fixed I2S pinout application code
* full model-only peripherals on `Lite`

## `14` Sources

### Official Product Material

* M5Stack product page: `https://m5stack.com/cardputerzero`
* Product pre-launch notes indicate that specifications are preliminary

### Local Reference Material

* `.tmp/M5CardputerZero-UserDemo/README.md`
* `.tmp/M5CardputerZero-UserDemo/projects/UserDemo/main/src/main.cpp`
* `.tmp/M5CardputerZero-UserDemo/projects/APPLaunch/main/src/main.cpp`
* `.tmp/M5CardputerZero-UserDemo/projects/APPLaunch/main/ui/components/ui_app_music.hpp`
* `.tmp/M5CardputerZero-UserDemo/projects/APPLaunch/main/ui/components/ui_app_setup.hpp`

### Cross-Check Reference

* Hackster coverage of the announced hardware:
  `https://www.hackster.io/news/m5stack-unveils-the-cardputerzero-an-all-in-one-pocket-sized-gadget-powered-by-the-raspberry-pi-cm0-1cdfe005d71d`
