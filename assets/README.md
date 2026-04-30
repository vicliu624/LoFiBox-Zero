<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Asset Baseline

This repository may reuse visual assets from the legacy `LoFiBox` project, but it does not inherit that project's code structure.

## Imported Sets

### `ui/icons/legacy-lofibox`

These are the small self-created PNG assets carried over from the author's legacy LoFiBox project at `C:\Users\VicLi\Documents\Projects\LoFiBox\images`.

- Intended use: candidate runtime UI icons and app artwork during early product development
- Current source set:
  - `About.png`
  - `Equalizer.png`
  - `Library.png`
  - `logo.png` (generated from the formal app logo in `data/io.github.vicliu624.lofibox.svg`)
  - `Music.png`
  - `NowPlaying.png`
  - `Playlists.png`
  - `Settings.png`
- Typical dimensions: `164x164`, with `logo.png` at `180x180`

The formal application logo source is `data/io.github.vicliu624.lofibox.svg`.
The `logo.png` file in this directory is the raster hicolor/runtime projection
of that source and should not be edited independently. Both projections must
preserve real alpha transparency; a flattened checkerboard preview or opaque
white export matte is not a valid LoFiBox logo asset.

## Current Rule

- Legacy project assets may be reused as visual inputs.
- Legacy project folders and architecture may not be revived as structural guidance for this repository.
- The product interpretation of these assets now lives in `docs/specification/legacy-lofibox-product-guidance.md`.
- If a later feature starts using one of these images at runtime, that usage should be recorded in `README.md` or the relevant feature documentation.

## License And Packaging Rule

- Installable project-created logo and UI icon assets are licensed as `GPL-3.0-or-later`.
- AppStream, desktop, and MIME metadata are licensed as `CC0-1.0`.
- All PNG resources in this repository are project-created assets and were not downloaded from third-party sources.
- `assets/ui/icons/legacy-lofibox/*.png` are installable project-created UI assets.
- Design prototypes are not runtime assets and should not be tracked in Git or installed into build artifacts unless a later product decision promotes them into governed source assets.
- Fonts are system dependencies, not vendored LoFiBox assets; packaged builds should use system `fonts-noto-cjk` or compatible CJK sans fonts.
- If any future asset has an external origin, split its copyright and license entry in `debian/copyright` and `docs/third-party.md` before distributing it.
