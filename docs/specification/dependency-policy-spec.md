<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Dependency Policy Specification

## 1. Purpose

This document defines dependency admission rules for `LoFiBox Zero`.
Dependencies must support the product goal of a complete Linux desktop music player that can enter Debian and Ubuntu official repositories.

## 2. Admission Checklist

Before adding a dependency, the change must check:

- license compatibility
- whether the dependency exists in the Debian archive
- whether it supports offline builds
- whether it introduces precompiled binaries or binary blobs
- whether it affects eligibility for the Debian `main` archive
- whether it can be represented in `debian/control` as `Build-Depends`, `Depends`, `Recommends`, or `Suggests`
- whether the dependency belongs globally or only to one provider/protocol module

## 3. Preferred Dependency Form

- C++ dependencies should use system libraries.
- Archive-available large third-party libraries must not be vendored.
- Dependencies such as `FFmpeg`, `GStreamer`, `TagLib`, `SQLite`, `libcurl`, `libsmbclient`, `libssh` or `libssh2`, `libnfs`, `libxml2`, `libupnp`, `Qt`, `GTK`, or `SDL2` must be discovered through CMake `find_package` or `pkg-config`.
- Protocol-specific dependencies must be localized to their provider or module.
- One optional protocol provider must not pollute the entire project dependency surface.

## 4. Runtime Helper Dependency Rule

Runtime helper scripts are application resources and may depend only on dependencies declared by the package.

- `tag_writer.py` is a metadata writeback helper and depends on `python3` plus `python3-mutagen`.
- `remote_media_tool.py` is a remote-source protocol bridge and currently depends only on `python3` standard-library modules.
- Helper dependencies must appear in `debian/control` as runtime `Depends`, `Recommends`, or provider-package dependencies according to whether the associated feature is mandatory or optional.
- Helper code must not invoke `pip`, download modules, vendor wheel files, or write dependencies into user data directories.
- Runtime availability checks may diagnose missing helpers or modules, but the fix path is packaging/dependency governance, not self-installation.

## 5. Vendoring Rule

Vendored code is exceptional.
If unavoidable, the repository must document:

- why vendoring is necessary
- upstream source
- exact license
- whether it can be built from source
- why it is not better satisfied by a Debian archive package
- confirmation that no precompiled binary blob is included

Vendored code and vendored assets must be reflected in copyright documentation.

## 6. Feature Completeness Rule

Feature completeness must not be managed by deleting capabilities.
Complexity must be managed through:

- clear dependency declaration
- module boundaries
- provider boundaries
- build options
- package split design
- protocol-specific tests

## 7. Forbidden Patterns

- build-time dependency downloads
- hidden package-manager invocations inside build scripts
- vendoring archive-available large libraries without governance
- making UI or playback depend directly on protocol libraries
- adding a dependency whose license or source is unknown
