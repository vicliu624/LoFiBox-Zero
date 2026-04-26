<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Release Process

This document tracks the intended release discipline for official archive readiness.

## Release Gates

A release candidate should satisfy:

- clean CMake configure/build/test
- no build-time network
- valid desktop file
- valid AppStream metadata
- lintian review
- autopkgtest smoke
- dependency policy review
- copyright/resource review
- packaging install/uninstall sanity

## Versioning

Release tags should be traceable by `debian/watch`.
The exact tag pattern must be chosen before first public archive submission.

## Archive Review Preparation

Before sponsor review, prepare:

- source package
- copyright audit
- dependency audit
- build log from clean chroot
- autopkgtest result
- lintian result
- current engineering check report
