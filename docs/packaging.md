<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Packaging

Normative archive rules live in `docs/specification/debian-official-archive-spec.md`.
The temporary pre-archive GitHub Pages APT repository is governed by
`docs/specification/github-pages-apt-repository-spec.md`.

## Product Artifacts

Two release artifacts share the same application core:

| Artifact | CMake profile | Debian package | Display |
| --- | --- | --- | --- |
| Cardputer Zero | `cardputer` | `lofibox-cardputer-zero` | framebuffer + evdev |
| Debian desktop | `debian` | `lofibox` | native Wayland |

The profiles force their display target and disable X11 and the other display
backend. TUI and WebUI remain available in both. The `developer` CMake profile
retains individually selectable adapters for development.

```sh
cmake --preset cardputer-release
cmake --build --preset cardputer-release-build

cmake --preset debian-wayland-release
cmake --build --preset debian-wayland-release-build
```

Build packages in separate clean source/build directories with Debian build
dependencies installed. The package build profile chooses both package identity
and CMake product profile:

```sh
# Debian desktop: lofibox_<version>_<arch>.deb
DEB_BUILD_OPTIONS=parallel=2 dpkg-buildpackage -us -uc -b

# Cardputer: lofibox-cardputer-zero_<version>_<arch>.deb
DEB_BUILD_OPTIONS=parallel=1 dpkg-buildpackage -us -uc -b -Ppkg.lofibox.cardputer
```

For cross compilation add `-aarm64` and install matching target build libraries.
Match the target distribution and architecture; an amd64 or Ubuntu validation
package is not a Cardputer deployment package. The two binary packages conflict
because they are alternative builds of the same application and share per-user
state and a runtime socket; install the package for the intended machine. This avoids apt silently selecting a display
backend through inherited session variables.

Cardputer's `lofibox-cardputer-zero` command goes through its APPLaunch wrapper, including
framebuffer/keyboard detection and appliance WebUI defaults. Debian's `lofibox`
command directly execs `lofibox-wayland` (CLI help/version also work without an
active Wayland session). Package smoke tests enforce this separation.

The Cardputer Store requires namespaced installation paths. Its CLI and TUI are
`/usr/bin/lofibox-cardputer-zero` and `/usr/bin/lofibox-cardputer-zero-tui`,
private binaries/helpers live in `/usr/lib/lofibox-cardputer-zero`, and assets
and plugins in `/usr/share/lofibox-cardputer-zero`. APPLaunch's desktop file and
PNG are named `lofibox-cardputer-zero`; its desktop entry uses the absolute PNG
path supported by APPLaunch and standard desktop validation. The generic
desktop/AppStream/MIME integrations are installed only by the Debian build.

Never reuse a debhelper completion stamp after changing source. Before a local
continuation build, clear staging and completion stamps with `dh_clean`; then
rebuild and compare the actual package payload with the intended source.
Package-level checks must verify the `.desktop` Exec target exists and is
executable, the icon resolves, private helper/asset paths match the namespace,
and the package contains the current application code. A successful compiler
run elsewhere does not verify the contents of an already-created `.deb`.

## Optional Real-Browser Regression

The default test suite does not require a browser. For the embedded WebUI and
native Wayland integration test on a Linux host, install test tools separately:
Chromium, Weston (headless backend), ffmpeg and Python's `websocket-client`.
Chromium, Weston and `websocket-client` are test tools; FFmpeg is also a playback
runtime dependency of both product packages.

Both Linux product packages build a private `/usr/lib/lofibox/lofibox-mp3-decoder`
with `libmpg123-dev` and `libsoxr-dev`; debhelper records the `libmpg123-0`
and `libsoxr0` runtime dependencies. Native MP3 decoding retains the source
rate internally and resamples to the shared 48 kHz stereo float32 PCM contract
before the product DSP and sink consume it.
The helper handles local 44.1 kHz stereo MP3 with HQ resampling to the product
rate and without ReplayGain adjustment. A bounded check of Xing/Info byte extent and trailing ID3v1/APEv2
tags keeps concatenated or unverified streams on the existing FFmpeg path.
Other formats, channel counts, sample rates and network streams also use
FFmpeg. Fallback replaces the helper process so PID, pipe and cleanup ownership
remain with the existing audio pipeline. Developer builds missing libmpg123 or
libsoxr continue to use FFmpeg; product builds require both unless the helper option is
explicitly disabled. Cross builds must set `PKG_CONFIG_LIBDIR` for the target
architecture, as `debian/rules` does.

```sh
cmake -S . -B build-browser -DCMAKE_BUILD_TYPE=Release \
  -DLOFIBOX_PRODUCT_PROFILE=debian -DBUILD_TESTING=ON -DLOFIBOX_BROWSER_TESTS=ON
cmake --build build-browser --target lofibox_zero_wayland
ctest --test-dir build-browser -R lofibox_webui_browser_smoke --output-on-failure
```

The test creates temporary HOME/XDG/media data and an isolated headless Chromium
profile, runs Weston without the user's desktop session, and records screenshots,
logs and `report.json` under `build-browser/browser-results`. Failed checks fail
CTest. The paced audio sink and fixture enrichment helper keep the run offline;
real hardware sound, remote services and device performance require separate
acceptance. The isolated headless test uses Chromium's `--no-sandbox` option.

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

The generated archive is written next to the repository as `lofibox_<upstream-version>.orig.tar.xz`. The helper derives the upstream version from `CMakeLists.txt` and validates that `debian/changelog` names the same upstream version before creating the archive. It excludes `debian/`, VCS metadata, build directories, and generated package artifacts. This is a local validation helper; actual archive uploads should use release-tag tarballs tracked by `debian/watch`.

`debian/source/options` also ignores local build/debug directories such as `build/`, `.tmp/`, `out/`, `runs/`, and `obj-*` so source package checks are not polluted by generated debug output or temporary media-debug artifacts. These directories are not project source and must not be represented in Debian source diffs.

## Local Package Validation Gate

Local package validation should use the same package-level gates as CI:

```powershell
scripts/run-debian-package-validation.ps1
```

The helper creates the local upstream tarball, builds the Debian package through the dedicated package-build image, runs `lintian`, and runs `autopkgtest` against the generated `.changes` file. It intentionally depends on a pre-existing local image and does not pull or build images implicitly. By default the final `lintian`/`autopkgtest` container is run with Docker networking disabled so routine validation proves the prepared package-build image is self-contained; pass `-AllowNetwork` only when intentionally refreshing or diagnosing the image.

## Preview APT Repository

Before LoFiBox is accepted into the official Debian archive, early users may
install from the LoFiBox-maintained preview APT repository published through
GitHub Pages.

User setup:

```sh
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

The repository must be signed and consumed through `Signed-By`. Do not use
`apt-key`.

Preview package versions should use a local suffix such as:

```text
X.Y.Z-1~lofibox1
```

This keeps future official Debian versions such as `X.Y.Z-1` able to supersede
the preview package naturally.

Publication is manual through:

```text
lofibox-apt/.github/workflows/publish.yml
```

The workflow requires protected GitHub secrets:

```text
LOFIBOX_APT_GPG_PRIVATE_KEY
LOFIBOX_APT_GPG_KEY_ID
```

Local repository generation from existing `.changes` files uses:

```sh
scripts/build-github-pages-apt-repository.sh \
  --suite trixie \
  --component main \
  --architectures amd64,arm64 \
  --output public \
  --gpg-key "$LOFIBOX_APT_GPG_KEY_ID" \
  --changes ../lofibox_<preview-version>_amd64.changes \
  --changes ../lofibox_<preview-version>_arm64.changes
```

The GitHub-hosted preview workflow builds `amd64` and cross-builds `arm64`.
Packaging must keep build-machine tools and target libraries separate: build
tools such as `pkgconf` are native build dependencies, while target development
libraries are resolved for the selected host architecture. `debian/rules` must
select host-triplet compilers whenever `DEB_BUILD_GNU_TYPE` differs from
`DEB_HOST_GNU_TYPE`. GitHub-hosted cross-build jobs may use
`DEB_BUILD_OPTIONS=nocheck` because they cannot execute target architecture test
binaries on the x86_64 runner. The cross-build environment must also install
target runtime libraries needed by `dh_shlibdeps`, including target
`libstdc++6`.

## Autopkgtest

The package smoke test is intentionally limited to installed runtime behavior and desktop metadata, but it is not marked `superficial` because it is the package-level gate for CLI availability and desktop/AppStream metadata validity:

```sh
deb_version="$(dpkg-parsechangelog -S Version)"
deb_arch="$(dpkg --print-architecture)"
autopkgtest "../lofibox_${deb_version}_${deb_arch}.changes" -- null
```

The local Docker-based package-build image excludes `/usr/share/man/*` through dpkg path-exclude rules, so autopkgtest must not assert manpage installation from the testbed filesystem. Manpage presence is validated through package contents and `lintian`.

## Validation Commands

Packaging work should support:

- `lofibox --help`
- `lofibox --version`
- `lofibox-tui --help`
- `dpkg-checkbuilddeps debian/control`
- `desktop-file-validate`
- `appstreamcli validate`
- `lintian`
- `autopkgtest`
- `sbuild` or `pbuilder`
- install/uninstall smoke tests

## Install Paths

The Debian desktop package installs into these standard paths (the Cardputer
namespace is described above):

- `/usr/bin/lofibox`
- `/usr/bin/lofibox-tui`
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

On Linux, package/runtime launches may have a sparse environment. If `HOME` is unset, LoFiBox should resolve `~` from the effective user's account record before using a temporary fallback, so APPLaunch and service-style starts share the same user state as shell commands for that user.
Package tests must not rely on the user's real `HOME`, real music library, real audio device, or external network.
