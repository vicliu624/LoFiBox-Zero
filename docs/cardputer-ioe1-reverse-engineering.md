<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Cardputer Zero indicator firmware reverse engineering

Analysis date: 2026-11-09. This work is an **offline experiment**, not a deployed
firmware change. The device has not been reset, placed in bootloader mode or
flashed. The installed LoFiBox icon build still defaults music flashing to off.

Follow-up on 2026-11-10: the player now runs the corrected PCM/backlight build
documented in [audio PCM validation](cardputer-audio-pcm-validation.md). No MCU
firmware change accompanied that player update. The exact I2C/MMIO distinction
and expanded offline protocol audit are recorded below.

## Result and evidence boundary

MCU source is not required to establish the current limitation or build a
binary patch prototype. The supplied schematic, the device's bundled firmware
headers, disassembly and instruction-level emulation establish a plausible
hardware PWM route. They do not establish that an experimental image can be
safely deployed or recovered on this physical device.

The factory bundle at
`/usr/share/factory_test/py32_firmware/cardputerzero_ioe1_upgrade.tar.gz`
contains C arrays of compiled firmware and a Python upgrader, not MCU source.
The minor-version 01 array produces a 20,480-byte image for
`0x08001000..0x08005fff`, SHA256
`babbca2690e61438950698969284443eb823a766998cdce11f56c9ea83bf7e15`.
Its version matches the device's reported `0xBA == 0x01`. The live MCU flash
has **not** been read back, so this is a vendor reference, not a device backup.
The bundled F8 image was also inspected; it is not installed or substituted.

The reference schematic is
`C154-CardputerZERO_SCH_V0.6.1_20260702.pdf`, SHA256
`91bbf37ae57e9153d9190367979a7789c225ff83567d2b1a9eba30fcd873a0d9`.
Pages 6 and 8 identify the M5IOE1 controller and the three WHITE indicators.
There is no RGB color control on these three components.

## Decoded native behavior

| Location in minor-01 reference | Observed behavior |
| --- | --- |
| `0x0800229c` | I2C register dispatcher, packet pointer and packet length |
| `0x080023be..0x080023c4` | Read bank `0x1b..0x20` is explicitly filled with six zero bytes |
| `0x08002c7a`, `0x08002c8c`, `0x08002cca` | BD/BE/BF handlers accept only values 0 through 3 |
| `0x080034b4` | Native blink timing, approximately 500 ms and 150 ms per toggle |
| `0x080033ec` | Pending native LED ownership/configuration processing |
| `0x080038f4` | Initializes GPIO descriptors, including the physical port/pin mapping |
| `0x0800222c` | Computes the shared TIM1 period and channel-4 backlight compare |
| `0x08002e1c` | Increments the existing millisecond tick |
| `0x0800151c` | Original deep STOP entry |

Emulation executes the original dispatcher and reproduces accepted 0/1/2/3
values and rejected 4/255 values. It also reproduces the six zero PWM read
bytes. The earlier physical I2C probes agree with this limitation. Increasing
the duration of a native slow-blink test cannot turn it into a brightness ramp.

Executing the original GPIO-descriptor initialization gives this mapping:

| Logical PWM | Indicator | M5IOE1 GPIO | MCU port/pin | Timer alternate function |
| --- | --- | --- | --- | --- |
| PWM1 | Fn / LED6 | PYG9 | PA0 | AF2, TIM1_CH1 |
| PWM2 | Shift / LED7 | PYG8 | PB0 | AF2, TIM1_CH2 |
| PWM3 | Sym / LED8 | PYG11 | PB2 | AF6, TIM1_CH3 |
| PWM4, existing | LCD backlight | PYG10 | PA7 | AF2, TIM1_CH4 |

The alternate functions and TIM1 address `0x40012c00` agree with the primary
[PY32F002B datasheet, alternate-function table, page 26](https://download.py32.org/Datasheet/en/PY32F002B_Datasheet_V1.8.pdf)
and [reference manual](https://download.py32.org/ReferenceManual/en/PY32F002B_Reference_Manual_V1.0.pdf).
This identifies a compatible register/pin model; the physical MCU die marking
has not been independently inspected. In particular, **PB2 requires AF6 for
CH3**; AF2 selects CH1N. The experiment does not change clock configuration.

## Isolated prototype

The current experimental payload is 1,016 bytes. It is linked into the verified
all-FF region `0x08005b78..0x08005ffb`, leaving the final four-byte checksum
outside the payload. Original code/data is retained apart from nine verified
BL callsites, the SysTick vector, the injected region and the recalculated CRC.
The reset vector and original C startup copy/zero table remain unchanged.

The image uses CRC-32/MPEG-2 (polynomial `0x04c11db7`, initial `0xffffffff`,
MSB-first, no final xor), stored little endian. The implementation reproduces
the checksums of both vendor 01 and F8 images before generating a candidate.
The builder pins the complete input SHA256 and every replaced BL instruction;
a different input is rejected. The bootloader at `0x08000000..0x08000fff` is
outside the image and is never included in the build output.

The experimental interface is:

- Read `0xc0`: signature `LBP1` so a host can distinguish this experiment from
  stock firmware. This is a private experimental extension, not an official API.
- Write/read `0x1b..0x20`: three little-endian 16-bit values, PWM1/PWM2/PWM3.
  Bits 0..11 are the requested duty, bit 14 selects inverted polarity, bit 15
  enables the channel. Reserved bits are masked out.
- Each channel expires after 1,000 ms without a corresponding byte update.
  Explicit disable restores its captured GPIO/timer configuration immediately
  on the next native LED service pass.
- Any native Shift/Fn/Sym mode yields all music channels. Original pending-LED
  handling runs before the experiment reconciles hardware ownership.
- The existing power/readiness gating is retained. TIM1 CEN and MOE must already
  be set. The experiment preserves PSC, ARR, CCR4, channel-4 mode/enable and the
  unrelated GPIO bits in the simulated cases.
- SysTick keeps its original increment and posts an ordinary native LED event
  every 16 ticks while PWM is requested or owned. It does not modify GPIO or
  perform PWM calculations inside SysTick. This avoids a permanently dirty
  event flag and provides periodic lease expiry even after a host process dies.
- PWM is generated by TIM1. A host would send a low-rate brightness envelope;
  it would not toggle lamp outputs from the Linux render/audio thread.
- Active music PWM uses shallow WFI in place of deep STOP so that the timer can
  continue. Inactive or power-gated states delegate to the original STOP routine.
  This is a material power-management change that requires further validation.

The experiment uses 48 bytes at `0x20000b00..0x20000b2f`, above the original
initial SP `0x20000a68`, within the compatible 3-KiB RAM model. The initialization
hook clears this region. No original stack, .data or .bss address is moved.

## Validation performed

Fourteen groups pass under Unicorn with Cortex-M instruction execution, mapped
RAM and inert MMIO. GPIO set/reset side effects are modeled for the tested
outputs. WFI is modeled as an immediate wake, not as a real interrupt or clock
transition. Tests cover:

1. Pinned image, replaced callsites, vector and checksum integrity.
2. State initialization and capability signature.
3. Original binary's missing PWM1-3 behavior and native mode bounds.
4. Multi-byte and individual-byte read/write, including reserved-bit masking.
5. Eighteen duty/polarity combinations, correct alternate functions and retained
   channel-4/backlight fields.
6. Explicit disable restoring GPIO and the three timer channels.
7. Sixteen-tick event cadence, one-second lease and no inactive event posting.
8. Lease arithmetic across 32-bit tick wrap.
9. Shift/Fn/Sym priority in all three active modes, followed by PWM resumption.
10. Original power/readiness and timer-enable gating.
11. Native register behavior and original upgrade-request flag behavior.
12. Active shallow-WFI selection and delegation of inactive/off STOP calls.
13. Restoration of the caller's PRIMASK state.
14. A 121-step cosine envelope producing monotonic rise/fall of timer compares.

The largest observed routine stack use is 432 bytes, excluding the main caller
and the hardware exception frame. This is not a worst-case firmware stack proof.
Normal-entry simulated cases observe at most 37 consecutive instructions with
interrupts masked. Division and full-channel iteration run with interrupts
enabled. Instruction counts are not measured MCU CPU time or I2C latency.

Compiler: `arm-linux-gnueabihf-gcc 13.3.0` (Ubuntu 24.04), Cortex-M0+, Thumb,
soft-float, `-Os -flto`, freestanding/no standard library. The local compiler
script, source, linker script, vendor reference images, candidate, patch manifest
and JSON test report are preserved together under
`.tmp/offline-artifacts/ioe1-re-20261109/`.

Current candidate SHA256:
`1c81cf5fee5387e8fc48eb279a726225c116bdfd2910d0201f26e579f5c9e58d`.
It is named `cardputer-ioe1-LBP1-OFFLINE-EXPERIMENTAL.bin` and is explicitly
marked **DO NOT FLASH** in the manifest.

## Outstanding qualification

The prototype is not yet eligible for a device flash. Required evidence still
includes the complete startup/main-loop/power-state behavior, real peripheral
clock readiness, IRQ preemption/interleavings, actual PWM brightness/polarity,
physical shutdown/reboot and measured MCU/I2C timing. The native-state tests
above are routine-level tests; they do not exercise the complete firmware FSM.

The bundled upgrader documents I2C app/bootloader addresses `0x4f`/`0x5f` and
the app upgrade request `0xfd=0x1a`. It has not been executed. If a patched app
cannot boot or answer I2C, entering bootloader and restoring the vendor image
has not been demonstrated. Possession of the same-version vendor image does
not establish a recovery path or equality to the currently installed image.
Changing the shared IO/power MCU requires a concrete recovery plan and explicit
approval before any flash attempt. Further offline analysis needs no user MCU
source and no repeated short visual lamp tests.

No production LoFiBox symbols were changed for this experiment. GitNexus cannot
classify the new ignored experimental symbols or the vendor binary; those
impact lookups return UNKNOWN, not LOW. There is no commit, package rebuild,
firmware upload or firmware write associated with this experiment.

## 2026-11-10: control register clarification and protocol audit

Two separate address spaces must be distinguished. Linux sends I2C commands
to the firmware at address `0x4f`; the MCU alone accesses its own memory-mapped
timer and GPIO registers. An internal register address cannot be substituted
for an I2C register number.

| Control | Firmware I2C register | Internal control already identified |
| --- | --- | --- |
| Shift indicator | `0xbd`, native mode 0..3 | PB0 AF2, TIM1 CH2, CCR2 `0x40012c38` |
| Fn indicator | `0xbe`, native mode 0..3 | PA0 AF2, TIM1 CH1, CCR1 `0x40012c34` |
| Sym indicator | `0xbf`, native mode 0..3 | PB2 AF6, TIM1 CH3, CCR3 `0x40012c3c` |
| Backlight | `0x21..0x22` | TIM1 CH4, CCR4 `0x40012c40` |
| Generic PWM1..3 bank | `0x1b..0x20` | The vendor reference supplies six constant zero read bytes; the tested writes do not set the three timer compares |

The internal controls support varying duty; they are not limited to four
brightness levels. The four values belong to the vendor's keyboard indication
protocol. Real breathing still requires a verified way to drive the internal
PWM controls from the running device.

The existing experimental patch also identifies TIM1 CCMR1 at `0x40012c18`,
CCMR2 at `0x40012c1c`, CCER at `0x40012c20`, PSC at `0x40012c28` and ARR at
`0x40012c2c`. CH1/2/3 share the timer with the backlight; changing its global
frequency is not a per-indicator brightness operation. The candidate preserves
PSC, ARR and the CH4 fields in its tested routine cases. GPIOA mode/AF registers
are `0x50000000`/`0x50000020`; GPIOB mode/AF registers are
`0x50000400`/`0x50000420`. These mappings agree with the vendor binary's existing
GPIO descriptors and the primary PY32 register documentation linked above.

An additional local-only script, `.tmp/ioe1_protocol_audit.py`, executed the
SHA256-pinned vendor reference dispatcher with inert MMIO:

- All 256 one-byte read selectors were executed.
- All 256 selectors were tested with each of `0x01`, `0x80` and `0xff` as a
  one-byte write value: 768 calls, with no emulation errors.
- Each full 16-bit PWM1/2/3 field was tested with eleven duty/enable/polarity
  variants; three six-byte multi-channel variants were added. All 36 cases
  read back zero and left the three indicator compares unchanged, including
  after running the original native LED service.
- The write/read dispatcher branches, including the `0xb0..0xbf` extension and
  `0xf0`/`0xfd` control commands, were inspected. No ordinary direct-MMIO write
  command or additional indicator-duty handler was identified in that audit.
  This does not prove every state, packet length, malformed packet or command
  sequence; the scan is not permission to sweep writes on the actual device.

The read response banks observed in the emulator were `0x00..0x2f`,
`0x70..0x8f`, `0xa0..0xa3`, `0xb0..0xbf` and `0xfb..0xff`. A changed generic
pending flag alone is not evidence that a write changed a PWM channel.

A new explicit MMIO model also executes reset and C startup through the first
main-loop entry at `0x080043cc`, rather than beginning at a patched routine.
With instantaneous modeled ADC ready/completion and fast-forwarded time-only
busy waits, vendor and candidate reach that point in 19,891 and 20,089 executed
instructions respectively. Their persistent globals through `0x20000667` and
their MMIO write sequences match while PWM is inactive. Prepared-RX-buffer
execution of the original I2C STOP handler also matches for tested native LED
and timer-frequency commands, exercising the real patched callsite.

The subsequent complete main-FSM attempt remains **INCOMPLETE**: it reaches
waiting loops that require asynchronous input/interrupt and peripheral behavior
not supplied by this model. Its instruction-limit observation is an emulator
coverage limit, not proof of a firmware fault. The earlier and expanded
simulation results do not qualify the candidate for flashing.

The user's schematic shows `G16_PY32_RST` and SWD/SWC test points on the
controller. The [official Cardputer Zero pin map](https://docs.m5stack.com/en/CardputerZero)
also connects G16 to NRST. The bundled upgrader's normal transition requires
the application to answer `0xfd=0x1a`; its recovery detection handles a device
already answering at bootloader address `0x5f`. Neither reference proves that
pulsing NRST forces bootloader entry after a valid-but-nonfunctional application
image. No reset or bootloader command was sent to the device.

Read-only live checks on 2026-11-10 still return minor version `0x01`, three
zero PWM1..3 words, the current backlight word, and a 20,000-Hz shared PWM
frequency. These checks agree with the vendor-reference limitation but do not
substitute for a live flash readback. The LoFiBox process and corrected ELF
build ID remain the same, with playback active and `dpkg -V` clean.

The follow-up scripts, JSON reports, incomplete-model record, live read-only
observations and unchanged candidate identity are preserved separately under
`.tmp/offline-artifacts/ioe1-audit-20261110/`. No production source, installed
package or firmware was changed by this follow-up.
