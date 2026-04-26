<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Cardputer Zero Adaptation Specification

## 1. Purpose

This document defines the `Cardputer Zero` adaptation profile for `LoFiBox Zero`.

`Cardputer Zero` is not the product's whole identity.
It is, however, a current first-class target profile that the project must adapt to deliberately.

This distinction prevents two opposite failures:

- shrinking LoFiBox into a board-specific demo
- ignoring a real target's special screen and interaction constraints because LoFiBox is also a Linux desktop application

The base LoFiBox product is already a chromeless desktop-widget music terminal.
The Cardputer Zero profile adapts that product shell to a stricter small-screen and hardware-keyboard environment; it does not invent the no-menu-bar rule by itself.

## 2. Authority

For final product meaning, use `lofibox-zero-final-product-spec.md`.
For architecture boundaries, use `project-architecture-spec.md`.
For page, layout, and visual rules, use:

- `lofibox-zero-page-spec.md`
- `lofibox-zero-layout-spec.md`
- `lofibox-zero-visual-design-spec.md`

This document controls Cardputer Zero profile-specific presentation and adapter constraints.

## 3. Profile Definition

The `Cardputer Zero` profile is a Linux target profile with:

- small-screen presentation
- hardware-keyboard-first input
- the shared chromeless application surface
- no conventional window menu bar
- no desktop chrome treated as product UI
- page-level discoverability through profile-appropriate help, such as `F1:HELP`

The profile may use framebuffer, VNC, PocketFrame, or another Linux presentation adapter during development or validation.
Those adapters are not product truth.

## 4. Screen Contract

Where the active target uses the Cardputer/PocketFrame small-screen profile, the UI must treat `320x170` as the logical design surface unless a later profile spec explicitly changes it.

The product surface must:

- use all available logical screen area intentionally
- avoid conventional desktop menu bars
- avoid OS window chrome as part of the design
- keep top-bar content inside the product's own visual system
- keep page affordances readable at the small-screen scale

## 5. Input Contract

The profile is hardware-keyboard-first.

Linux input truth still comes from kernel input events such as `EV_KEY` and `KEY_*`.
Printed key legends, product images, PocketFrame button labels, or simulator naming must not redefine key semantics.

Profile input adapters may translate physical events into logical app commands, but that translation must stay inside the platform/device boundary.

## 6. Product Model Non-Fork Rule

The `Cardputer Zero` profile must reuse the same product semantics as other Linux targets:

- `Track`
- `Album`
- `Artist`
- `Playlist`
- `Station`
- `Queue`
- `PlaybackSession`
- `EqProfile`
- source profiles
- metadata
- lyrics
- artwork
- DSP chain

The profile may change presentation.
It must not fork playback, library, streaming, metadata, credentials, persistence, or DSP behavior.

## 7. Validation Rule

Profile-specific changes should be validated against a real profile runtime or a faithful Linux validation harness.

The validation harness must remain an adapter.
If a behavior works only in the harness and not in the shared Linux product code path, the implementation is structurally suspect.

## 8. AI Constraints

- Do not remove Cardputer Zero constraints merely because LoFiBox is a Linux desktop player.
- Do not reclassify Cardputer Zero constraints as product-wide desktop constraints.
- Do not add window menu bars or generic desktop chrome to the Cardputer Zero product surface.
- Do not describe the no-menu-bar rule as only a Cardputer Zero workaround; it is part of the primary LoFiBox shell.
- Do not let framebuffer, VNC, PocketFrame, or container details leak into shared app semantics.
