<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Testing And CI Specification

## 1. Purpose

This document defines the test and CI requirements for `LoFiBox Zero`.
Tests exist to preserve product behavior, architecture boundaries, packaging readiness, and Debian/Ubuntu archive suitability.

## 2. Unit Tests

Unit tests should cover:

- core state machine
- playback queue
- application command/query services returning structured command results without depending on GUI page selection state
- runtime command/query contract behavior for accepted/applied result semantics, correlation ids, versioning, and invalid command rejection
- runtime domain behavior for playback, active queue, active EQ, active remote session, live settings, and full snapshot assembly without going through GUI pages
- runtime snapshots for playback, active queue, active EQ, and active remote session before GUI row or terminal projection
- remote browse query service behavior for root browsing, child browsing, search, stream resolution, degraded facts, directory cache reuse, and local read-only remote fact caching
- credential command service behavior for secret status, set, delete, safe redaction assumptions, and separation from source-profile field editing
- cache and runtime diagnostic command services returning structured facts before terminal or UI projection
- playlist parsers
- metadata parser behavior
- EQ and biquad behavior
- XDG path resolver
- Unicode-safe text editing for Search and other text-entry fields
- invalid UTF-8 rejection or repair before committed text enters app state

## 3. Integration Tests

Integration tests should cover:

- GUI command routing translating page events into application commands without duplicating product behavior
- GUI live playback, queue, and EQ actions submitting runtime commands instead of directly mutating live runtime truth
- desktop-open URL playback submitting runtime commands instead of directly starting remote streams from GUI runtime code
- in-process runtime command dispatcher behavior for playback transport, queue stepping, EQ mutation, snapshot queries, and serialization/version increments
- direct command service behavior for library/source/credential/cache/diagnostic domains under isolated XDG test roots
- first-stage direct CLI parsing/formatting for source list/add/update/probe, credentials set/status/delete, library scan/status/query, cache status/clear, and doctor
- runtime command client/server behavior before live playback or active queue control is exposed to external command consumers
- runtime transport-envelope behavior at the contract level before any Unix socket, D-Bus, JSON-RPC, stdio, or named-pipe implementation is selected
- local library scan
- database migration
- metadata cache
- direct URL stream using a controlled local fixture server
- mock playback through `NullAudioOutput`
- local and remote search result grouping over normalized media items
- GUI Remote Browse, Server Diagnostics, Stream Detail, and Search consuming application-service query results instead of calling remote providers directly
- Search matching and display with non-ASCII metadata fixtures

## 3.1 Platform Input Adapter Tests

Platform input tests should cover:

- X11 committed UTF-8 text delivery from the text adapter where test infrastructure permits it
- preedit data remaining transient and not mutating committed query state
- framebuffer/evdev direct-key translation boundaries
- framebuffer/evdev not being advertised as system CJK IME support

These tests must use controlled adapters or fixtures.
They must not require the CI environment to run a user's real input-method daemon.

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
- depend on a real user input-method configuration
- simulate product commands by driving GUI-only selected-row or field-editor internals when an application command service is the intended boundary
- simulate live runtime commands by driving GUI-only page state when the runtime command bus is the intended boundary
- test remote diagnostics or stream detail by asserting only rendered row text when structured application query facts are available
- test playback, queue, EQ, or active remote-session truth by asserting only rendered row text when runtime snapshots are available
- mutate live playback or active queue from a second process without an explicit runtime command path

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
