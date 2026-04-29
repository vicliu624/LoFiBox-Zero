<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Third-Party Code And Resources

Normative copyright rules live in `docs/specification/copyright-resource-governance-spec.md`.

This file is the human-maintained register for third-party code and resources.

## Required Fields

Each entry must include:

- name
- upstream source
- version or snapshot
- license
- reason for inclusion
- whether it is vendored
- whether Debian already packages it
- whether any binary blob is present

## Current Entries

### `src/third_party/stb_image.h`

- name: `stb_image`
- upstream source: `https://github.com/nothings/stb`
- version or snapshot: `v2.30`, as stated in the vendored header
- license: `MIT OR Unlicense`, as stated at the end of the header
- reason for inclusion: PNG/image loading for host runtime artwork paths
- vendored: yes
- Debian already packages it: needs confirmation before archive upload
- binary blob present: no
- warning policy: compiler warnings from the implementation header are locally suppressed only around the `stb_image.h` include site; project-owned code must not disable warnings globally to hide third-party noise

### System CJK font dependency

- name: `Noto Sans CJK` or compatible CJK sans font
- source: system font packages
- license: handled by the distribution package, commonly `OFL-1.1` for Noto fonts
- reason for use: CJK text rendering
- vendored: no
- Debian package: prefer `fonts-noto-cjk`
- binary blob present in this repository: no

## Project-Created Asset Domains

### `data/io.github.vicliu624.lofibox.svg`

- license: `GPL-3.0-or-later`
- packaging status: installable
- audit note: treated as the formal project-created application logo asset

### `assets/ui/icons/legacy-lofibox/*.png`

- license: `GPL-3.0-or-later`
- packaging status: installable runtime UI assets
- audit note: `logo.png` is generated from the formal application logo SVG; other PNG resources in this directory are self-created legacy runtime UI assets and not downloaded from third-party sources

## Metadata And Fixture Domains

- `data/*.desktop`, `data/*.metainfo.xml`, and `data/mime/*.xml`: `CC0-1.0`
- generated test fixtures: `CC0-1.0`
- commercial songs, album covers, downloaded lyrics, and user music files: forbidden as test fixtures or AppStream screenshot content

## Remaining Audit Status

The repository still requires a full third-party audit before official archive submission.
Known areas to audit include:

- whether `src/third_party/stb_image.h` should remain vendored or use a system package
- future design prototypes, if any, should remain outside the Git repository unless they become governed source assets
- future screenshots
- future generated or downloaded artwork
