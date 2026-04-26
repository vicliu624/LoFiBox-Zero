<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Testing And CI Specification

## 1. Purpose

This document defines the test and CI requirements for `LoFiBox Zero`.
Tests exist to preserve product behavior, architecture boundaries, packaging readiness, and Debian/Ubuntu archive suitability.

## 2. Unit Tests

Unit tests should cover:

- core state machine
- playback queue
- playlist parsers
- metadata parser behavior
- EQ and biquad behavior
- XDG path resolver

## 3. Integration Tests

Integration tests should cover:

- local library scan
- database migration
- metadata cache
- direct URL stream using a controlled local fixture server
- mock playback through `NullAudioOutput`

## 4. Protocol Tests

Protocol tests should use mocks or local fixtures for:

- SMB provider behavior where feasible
- WebDAV provider behavior
- Subsonic and OpenSubsonic mock server
- Jellyfin mock API
- DLNA and UPnP discovery where feasible

Tests must not depend on the user's real servers.

## 5. Packaging Tests

Packaging tests must include:

- `lofibox --version`
- `lofibox --help`
- desktop file validation
- AppStream metadata validation
- install and uninstall sanity

`debian/tests/control` and `debian/tests/smoke` must exist for autopkgtest.

## 6. Test Isolation Rules

Tests must not:

- depend on a real user music library
- depend on real network services
- depend on real audio devices
- pollute the user's `HOME`
- access the external internet

Tests must support CI and Debian autopkgtest environments.

## 7. CI Requirements

CI should include:

- Ubuntu latest build
- Debian unstable or sid container or chroot build
- CMake configure, build, and test
- `clang-tidy` or `cppcheck`
- format check
- `desktop-file-validate`
- `appstreamcli validate`
- `lintian`
- autopkgtest smoke
- `sbuild` or `pbuilder` build validation

CI failures must not be solved by skipping tests.
They must drive boundary, dependency, packaging, or implementation fixes.
