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
- runtime host ownership behavior proving live runtime can run without `AppRuntimeContext` owning or constructing runtime internals
- runtime host tick serialization through the runtime command bus, proving that
  external playback commands cannot race the host tick loop against the same
  playback backend or session facade
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
- runtime transport-envelope behavior and the Unix socket runtime transport, including request/response framing, server shutdown, and snapshot consistency
- runtime CLI behavior through external transport only, with no in-process fallback and no dependency on `AppRuntimeContext`, controllers, runtime domains, runtime bus, or runtime server
- runtime command coverage for every published command kind, including stop, seek, queue jump/clear, remote reconnect, live settings apply, runtime shutdown, and runtime reload
- local library scan
- database migration
- metadata cache
- direct URL stream using a controlled local fixture server
- mock playback through `NullAudioOutput`
- local and remote search result grouping over normalized media items
- GUI Remote Browse, Server Diagnostics, Stream Detail, and Search consuming application-service query results instead of calling remote providers directly
- Search matching and display with non-ASCII metadata fixtures

Product-path integration tests must cover the real user routes that join
domains together, not only isolated class smoke tests. The minimum product-path
coverage is:

- selecting or starting a local library item results in an active playback
  session and a `RuntimeSnapshot.playback.status` of `PLAYING` or `PAUSED`
  with the expected `current_track_id`
- backend start refusal and inactive resume/toggle paths are covered so
  `PLAYING` cannot be projected when `audio_active=false`
- starting the already-active track id is covered as an idempotent runtime
  command that does not restart or tear down the backend
- local Linux playback backend smoke coverage must distinguish process-spawn
  acceptance from confirmed audio output; a decoder/sink path that exits before
  first output confirmation is a failed start, not successful playback
- selecting or starting a remote library/search item enters playback through a
  resolved remote-library-track path, not an empty local file path
- queue step/jump to a remote item without a remote resolver returns
  `applied=false` rather than corrupting playback state
- runtime snapshot queue projection includes displayable title/artist/source
  facts for visible queue items
- metadata and lyrics enrichment for remote items uses stable remote identity
  and cache keys instead of synthetic empty paths
- CLI field filtering and JSON errors are covered through installed-binary style
  invocations, including invalid fields and invalid command arguments
- TUI render tests include active playback snapshots for wide, normal, compact,
  micro, and tiny layouts, not only disconnected fallbacks
- non-ASCII title/artist fixtures are rendered in CLI and documentation capture
  paths with fonts that can display CJK/Japanese text
- product logo asset tests verify real alpha transparency, transparent corners,
  retained opaque foreground artwork, and scaled blitting that preserves the
  destination background behind transparent logo pixels

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
- `lofibox-tui --help` in package-level smoke tests when the package is built with the TUI target
- desktop file validation
- AppStream metadata validation
- install and uninstall sanity

`debian/tests/control` and `debian/tests/smoke` must exist for autopkgtest.

CI install-skeleton validation may configure Linux framebuffer and X11 presentation targets off for dependency isolation,
but it must keep the terminal-native TUI enabled. The install-skeleton job must build `lofibox_zero_tui_bin` before
`cmake --install` and assert the installed `lofibox-tui` executable alongside desktop, AppStream, icon, MIME,
documentation, helper, and manpage installation. Full package smoke tests remain responsible for proving the complete
installed executable surface, including `lofibox` and `lofibox-tui`.

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
- simulate runtime CLI by constructing an in-process app/runtime instance instead of going through `RuntimeCommandClient` and runtime transport
- assert runtime-host extraction by only checking that a few member pointers moved; tests must prove ownership, tick, callback removal, and client-only GUI access
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

Smoke tests that only prove construction, static rendering, or help output are
not sufficient as release confidence. Before publishing packages or refreshing
public documentation screenshots, a real Linux target smoke run must install the
built package over any older installed LoFiBox version, launch the runtime, play
at least one local track and one remote track where configured, query runtime
JSON, render TUI once in active playback state, and capture GUI/TUI/CLI evidence
from that installed binary.

GUI capture evidence must be cursor-clean. The X11 application window and any
VNC/PocketFrame root display used for the capture must hide their pointer cursor
before screenshots are accepted. A screenshot containing the VNC/X11 cross cursor
or pointer overlay is a failed visual smoke artifact, even if the product pixels
behind it are otherwise correct.
