<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Packaging

Normative archive rules live in `docs/specification/debian-official-archive-spec.md`.

## Standard Build

```sh
cmake -B build -S .
cmake --build build
cmake --install build --prefix /usr
```

## Debian Source Package Target

The repository should provide:

- `debian/changelog`
- `debian/control`
- `debian/copyright`
- `debian/rules`
- `debian/install`
- `debian/watch`
- `debian/source/format`
- `debian/tests/control`
- `debian/tests/smoke`

`debian/source/format` must be `3.0 (quilt)`.

## Local Debian Build Dependency Check

The repository provides a no-pull helper for checking declared Debian build dependencies against an already available local image:

```powershell
scripts/check-debian-builddeps.ps1
```

The helper intentionally refuses to build or pull images. If it fails, refresh or replace the local image explicitly before attempting `dpkg-buildpackage`.

## Local Debian Package Build Image

The package build image is intentionally separate from runtime and device images:

```powershell
scripts/build-debian-package-image.ps1
scripts/run-dpkg-buildpackage.ps1
```

`scripts/build-debian-package-image.ps1` requires the local base image `lofibox-zero/dev-container:trixie` to exist first. It may update apt metadata and install package-build tooling inside the derived image. `scripts/run-dpkg-buildpackage.ps1` does not install packages; it only validates declared build dependencies and runs `dpkg-buildpackage -us -uc -b` by default.

## Local Source Package Flow

Before an upstream release tarball exists, a local source-package check can create the `3.0 (quilt)` upstream tarball from the current working tree:

```powershell
scripts/create-orig-tarball.ps1
scripts/run-dpkg-buildpackage.ps1 -BuildArgs "-us -uc"
```

The generated archive is written next to the repository as `lofibox_0.1.0.orig.tar.xz`. It excludes `debian/`, VCS metadata, build directories, and generated package artifacts. This is a local validation helper; actual archive uploads should use release-tag tarballs tracked by `debian/watch`.

`debian/source/options` also ignores local build/debug directories such as `build/`, `.tmp/`, `out/`, `runs/`, and `obj-*` so source package checks are not polluted by generated debug output or temporary media-debug artifacts. These directories are not project source and must not be represented in Debian source diffs.

## Local Package Validation Gate

Local package validation should use the same package-level gates as CI:

```powershell
scripts/run-debian-package-validation.ps1
```

The helper creates the local upstream tarball, builds the Debian package through the dedicated package-build image, runs `lintian`, and runs `autopkgtest` against the generated `.changes` file. It intentionally depends on a pre-existing local image and does not pull or build images implicitly. By default the final `lintian`/`autopkgtest` container is run with Docker networking disabled so routine validation proves the prepared package-build image is self-contained; pass `-AllowNetwork` only when intentionally refreshing or diagnosing the image.

## Autopkgtest

The package smoke test is intentionally limited to installed runtime behavior and desktop metadata, but it is not marked `superficial` because it is the package-level gate for CLI availability and desktop/AppStream metadata validity:

```sh
autopkgtest lofibox_0.1.0-1_amd64.changes -- null
```

The local Docker-based package-build image excludes `/usr/share/man/*` through dpkg path-exclude rules, so autopkgtest must not assert manpage installation from the testbed filesystem. Manpage presence is validated through package contents and `lintian`.

## Validation Commands

Packaging work should support:

- `lofibox --help`
- `lofibox --version`
- `dpkg-checkbuilddeps debian/control`
- `desktop-file-validate`
- `appstreamcli validate`
- `lintian`
- `autopkgtest`
- `sbuild` or `pbuilder`
- install/uninstall smoke tests

## Install Paths

The package should install into standard paths:

- `/usr/bin/lofibox`
- `/usr/lib/lofibox/`
- `/usr/lib/lofibox/tag_writer.py`
- `/usr/lib/lofibox/remote_media_tool.py`
- `/usr/lib/lofibox/plugins/`
- `/usr/share/lofibox/`
- `/usr/share/applications/io.github.vicliu624.lofibox.desktop`
- `/usr/share/metainfo/io.github.vicliu624.lofibox.metainfo.xml`
- `/usr/share/icons/hicolor/scalable/apps/io.github.vicliu624.lofibox.svg`
- `/usr/share/man/man1/lofibox.1`
- `/usr/share/doc/lofibox/`

## Runtime Paths

Installed package paths are not writable application state. Runtime data must follow XDG conventions:

- config: `$XDG_CONFIG_HOME/lofibox` or `~/.config/lofibox`
- data: `$XDG_DATA_HOME/lofibox` or `~/.local/share/lofibox`
- cache: `$XDG_CACHE_HOME/lofibox` or `~/.cache/lofibox`
- state: `$XDG_STATE_HOME/lofibox` or `~/.local/state/lofibox`
- runtime: `$XDG_RUNTIME_DIR/lofibox` for ephemeral locks where available

Package tests must not rely on the user's real `HOME`, real music library, real audio device, or external network.
