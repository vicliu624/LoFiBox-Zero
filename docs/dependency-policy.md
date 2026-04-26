<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Dependency Policy

Normative dependency rules live in `docs/specification/dependency-policy-spec.md`.

## Admission Rule

Before adding a dependency, verify:

- license compatibility
- Debian archive availability
- offline build support
- absence of precompiled blobs
- impact on Debian `main`
- representation in `debian/control`
- whether the dependency is global or provider-local

## Preferred Mechanism

Use system libraries discovered through CMake `find_package` or `pkg-config`.

Dependencies such as FFmpeg, GStreamer, TagLib, SQLite, libcurl, libsmbclient, libssh/libssh2, libnfs, libxml2, libupnp, Qt, GTK, or SDL2 must not be hidden behind build scripts.

## Runtime Helper Dependencies

Python helper scripts are packaged runtime resources, not build-time dependency installers.

- `scripts/tag_writer.py` requires the system packages `python3` and `python3-mutagen`.
- `scripts/remote_media_tool.py` may use only the Python standard library unless a new dependency is admitted through this policy.
- Python helper dependencies must be represented in `debian/control`; they must not be installed through `pip` or downloaded during configure, build, install, tests, or first run.
- Runtime code may check helper availability and produce diagnostics, but it must not silently fetch, vendor, or install missing Python modules.

## No Build-Time Network

The build must not run dependency-fetching commands such as:

- `git clone`
- `curl`
- `wget`
- `pip install`
- `npm install`
- `cargo fetch`
- `go mod download`

## Vendoring

Vendored code is exceptional and must document upstream, license, source-build status, reason for inclusion, and binary-blob absence.
