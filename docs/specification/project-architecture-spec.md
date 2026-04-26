<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Project Architecture Specification

## 1. Purpose

This document defines the enduring architecture and ownership boundaries for `LoFiBox Zero`.

It does not describe current progress.
It describes what kinds of code and responsibilities belong in which layers, regardless of delivery stage.

For final product meaning, use `lofibox-zero-final-product-spec.md`.
For current implementation progress, use `implementation-status.md`.

## 2. Enduring Distinctions

- `core`
  Player domain model, commands, errors, configuration model, and platform-neutral primitives.
- `app`
  Product-facing orchestration and view-model behavior that survives platform changes.
- `playback`
  Playback lifecycle, playback session, queue, seek, gapless, crossfade, and playback-mode semantics.
- `audio`
  Decoder, output, DSP, EQ, `ReplayGain`, sample-rate conversion, and audio-device responsibilities.
- `metadata`
  Tags, artwork, lyrics identity linkage, metadata cache, and metadata normalization.
- `library`
  Library scanning, indexing, database migrations, queries, and readiness projection.
- `playlist`
  Playlist model and playlist-format parsing such as `M3U`, `M3U8`, `PLS`, and `XSPF`.
- `remote`
  Remote-source and media-server providers for direct URLs, radio, shares, and server protocols.
- `plugins`
  Provider registration, provider manifests, host API, and provider lifecycle boundaries.
- `desktop`
  Linux desktop integration such as MPRIS, D-Bus, media keys, and notifications.
- `platform`
  XDG paths, file watching, Linux runtime adapters, input/output surfaces, and platform differences.
- `security`
  Credential storage, TLS policy, authentication boundaries, and sensitive-information handling.
- `ui`
  Windows, pages, controls, view models, themes, and projection of product state.
- `product shell`
  The chromeless, no-menu-bar product surface used by LoFiBox as a desktop-widget music terminal.
- `platform/host`
  Runtime capability adapters that expose one unified metadata, artwork, playback, logging, caching, enrichment, tag-writing, and connectivity surface to the shared app.
- `platform/device`
  Linux integrations that exist for device-profile runtimes: framebuffer output, Linux input, and device-specific presentation.
- `device profile`
  A presentation and adapter profile for a specific Linux target form factor, such as `Cardputer Zero`; it may constrain screen size, chrome, input translation, and shell behavior, but it may not redefine product objects or playback semantics.
- `targets`
  Thin entry points that bind one platform runtime to the shared app.
- `container tooling`
  Reproducible Linux build/run environment. It is a delivery and validation shell, not a product behavior layer.
- `process instance lock`
  Host startup guard that prevents multiple LoFiBox processes for the same user/runtime from owning playback and device resources at the same time.

## 3. Removed Distinctions

- There is no SDL desktop simulator layer.
- There is no simulator shell truth.
- There is no separate simulator audio path.

Any future visual or input validation must go through a real Linux product target, a device-profile target, or explicit shared-layer tests.

## 4. Invalid Distinctions

- Do not split the project around the old `PlatformIO`, `Arduino`, or board-class patterns.
- Do not elevate framebuffer driver names, Linux device paths, container paths, or mock shell geometry into shared app concepts.
- Do not create fake generic concepts such as `audio_service`, `playlist_manager`, or `library_store` before their responsibility boundary is explicit in the specs.
- Do not reintroduce an emulator/simulator as if it were product truth.
- Do not let Cardputer Zero, PocketFrame, VNC, framebuffer, X11, Wayland, or container execution define the product's core object model.
- Do not confuse “not product identity” with “not supported”; `Cardputer Zero` is a required current adaptation profile.
- Do not add a conventional application menu bar to the primary product surface merely because LoFiBox is a Linux desktop application.

## 5. Stable Engineering Decisions

- Language: `C++20`
- Build system: `CMake`
- Product runtime family: Linux desktop application with device-profile adapters where needed
- Primary shell form: chromeless desktop-widget music terminal with no traditional application menu bar
- Standard build entry: `cmake -B build -S .`, `cmake --build build`, and `cmake --install build --prefix /usr`
- Shared small-screen profile: `320x170` where the active target uses the Cardputer/PocketFrame profile
- `Cardputer Zero` profile surface: same chromeless product shell, hardware-keyboard-first, small-screen-first
- Device-side Linux integration: direct framebuffer and `evdev` adapters behind our own device boundary
- Linux dev-container family: Debian `Trixie` userland for reproducible Linux builds
- Official distribution target: Debian and Ubuntu archive-compatible source package

## 6. Architecture Contract

- UI code may depend on view models, logical canvas primitives, colors, text drawing, frame timing, and logical input events.
- UI/page code must not include concrete platform adapters, backend protocol layers, desktop integration layers, security internals, playback internals, metadata internals, audio internals, or remote-provider internals.
- Shared app code may depend on unified capability interfaces such as metadata, artwork, lyrics, audio-playback, tag-writing, and connectivity providers.
- Shared app code may not depend on Linux headers, direct hardware paths, Docker/container facts, or desktop GUI libraries.
- Shared app code must not include concrete `platform/host`, `platform/device`, or `platform/x11` adapters except for explicitly thin app-runner binding code.
- `core` must not include `app`, `platform`, `audio`, `metadata`, `library`, `playback`, `remote`, `desktop`, `security`, or `ui`.
- UI code must not directly call `FFmpeg`, `GStreamer`, `SMB`, `Jellyfin`, `SQLite`, `D-Bus`, or other low-level backend/protocol details.
- `playback` must not know `Jellyfin`, `SMB`, `Subsonic`, `DLNA`, or other protocol details.
- Remote-source code must obtain media objects and media streams; it must not own player state-machine semantics.
- `metadata` and `library` must not depend on UI.
- `desktop` integration may translate desktop events into player commands; it must not redefine core product state.
- Host adapters may implement `RuntimeServices` and helper/resource resolution, but they must not include concrete page implementations or the concrete `LoFiBoxApp` type.
- Device/X11 presentation adapters may translate platform input/output into app-facing interfaces, but they must not depend on host runtime, concrete pages, or concrete app construction details.
- `targets` may compose app runners, host runtime services, asset loaders, presenters, and command-line parsing; they must not include page implementations or own product behavior.
- Protocol clients must not directly persist plaintext passwords, tokens, API keys, cookies, or authentication headers.
- Runtime user data must not be written to `/usr`, `/opt`, installation directories, or the current working directory.
- Runtime paths must follow XDG:
  - config: `~/.config/lofibox/`
  - data: `~/.local/share/lofibox/`
  - cache: `~/.cache/lofibox/`
  - state: `~/.local/state/lofibox/`
- Shared app code must not own process singleton behavior; single-instance enforcement belongs in host startup code and must guard the Linux executable target.
- Linux executable targets must be thin target bindings over shared product code.
- Device-profile targets must be compatible with framebuffer-oriented deployment when that profile is active.
- A device-profile target may remove desktop window chrome from the presented product surface, but it must not fork business logic, media handling, playback, metadata, or DSP behavior.
- The primary desktop target must also preserve the chromeless no-menu-bar product shell unless a future specification explicitly defines a different secondary shell.
- The container exists for reproducible development and validation, not as a second app runtime.
- Third-party device wrappers may be used only behind our own device adapter; their framework shape may not leak into shared app structure.
- Real device keyboard input belongs in `platform/device` as Linux `evdev` translation into logical app input events.
- Linux `EV_KEY` / `KEY_*` codes are the device input truth for Linux targets; printed key legends, product images, and external test harness button names may not redefine app input semantics.
- A physical key that the kernel reports as `KEY_C` is character/input key `C` unless the Linux input driver or device adapter receives an actual `KEY_RIGHT` event.
- Legacy product meaning must be interpreted through `legacy-lofibox-product-guidance.md`, not by reviving old repository structure.
- Media-format responsibilities must follow `lofibox-zero-media-pipeline-spec.md`.
- Track identity and fingerprint-backed enrichment responsibilities must follow `lofibox-zero-track-identity-spec.md`.
- Shared cross-page application-state responsibilities must follow `lofibox-zero-app-state-spec.md`.
- Persistence domains, hydration, and repair behavior must follow `lofibox-zero-persistence-spec.md`.
- Credential references, secure secret storage, and runtime session handling must follow `lofibox-zero-credential-spec.md`.
- Library scan, index build, refresh, and rebuild behavior must follow `lofibox-zero-library-indexing-spec.md`.
- Saved source selection and source-management projections must follow `lofibox-zero-app-state-spec.md` rather than being owned page by page.
- EQ and DSP responsibilities must follow `lofibox-zero-audio-dsp-spec.md`.
- Streaming responsibilities must follow `lofibox-zero-streaming-spec.md`.
- Debian official-archive responsibilities must follow `debian-official-archive-spec.md`.
- Dependency responsibilities must follow `dependency-policy-spec.md`.
- Provider and plugin responsibilities must follow `plugin-provider-system-spec.md`.
- Linux desktop integration responsibilities must follow `linux-desktop-integration-spec.md`.
- Cardputer Zero profile responsibilities must follow `cardputer-zero-adaptation-spec.md`.
- Testing and CI responsibilities must follow `testing-ci-spec.md`.
- Copyright and resource responsibilities must follow `copyright-resource-governance-spec.md`.
- Current UI implementation work must follow `lofibox-zero-page-spec.md`, `lofibox-zero-layout-spec.md`, and `lofibox-zero-visual-design-spec.md`.
- Final product meaning belongs to `lofibox-zero-final-product-spec.md`, not to any current implementation contract.

## 7. Minimal Externalization

- `src/core` is the truth layer for shared rendering primitives.
- `src/app` is the truth layer for app composition and logical input handling.
- `src/ui` owns page renderers, shared UI primitives, visual themes, and UI projection structs such as `UiAssets`, `SpectrumFrame`, and `LyricsContent`.
- `src/ui` must not include `src/app` headers or reference `app::` internal types; UI can only render view/projection data already translated at the boundary.
- `src/app` may compose `src/ui` renderers for the current monolithic app surface, but page drawing, chrome drawing, modal drawing, and low-level font/color drawing code must not live under `src/app`; app/runtime/playback state must be converted into UI projection structs before reaching page renderers.
- `src/app/app_input_router.*` owns page-level input routing from `InputEvent`/`UserAction` to app commands; `LoFiBoxApp` delegates input handling to this router and must not directly include `app/input_actions.h`.
- `src/app/app_page_model.*` owns page titles, current page rows, Settings rows, browse-list classification, and list viewport limits; `LoFiBoxApp` must not directly depend on `ui/ui_theme.h`, `ui/ui_primitives.h`, or define page-model branches itself.
- `src/app/app_renderer.*` owns page-level render routing and UI view-model assembly; `LoFiBoxApp` delegates `render()` through a narrow render target interface and must not directly include `ui/pages/*`.
- `src/app/app_lifecycle.*` owns application tick ordering: runtime status refresh, media-library loading, boot-page transition, and playback update; `LoFiBoxApp` delegates `update()` through a narrow lifecycle target interface.
- `src/playback`, `src/audio`, `src/metadata`, `src/library`, `src/playlist`, `src/remote`, `src/plugins`, `src/desktop`, `src/platform`, `src/security`, and `src/ui` are the target semantic structure for long-term source ownership.
- `src/platform/host` and `src/platform/device` are adapter layers only.
- `src/targets` must stay thin.
- `src/platform/host/single_instance_lock.*` is the host-owned process lifecycle guard for the Linux executable target.
- `tests` validate shared behavior without pulling in the device adapter.
- `docker` and `scripts/run-dev-container.*` own container execution details.
- `scripts/check-architecture-boundaries.ps1` is the repository-level dependency-direction gate and must be updated whenever this specification deliberately changes layer boundaries.

## 8. AI Constraints For Future Work

- New platform-specific details must stay in the corresponding adapter layer.
- Do not bypass `scripts/check-architecture-boundaries.ps1` to make a local feature compile; if it fails, either fix the dependency direction or first update this specification with a justified boundary change.
- Container-specific tooling must stay in scripts and documentation; it must not leak Docker concepts into shared app or device code.
- Vendor Linux hardware wrappers, if reintroduced, must stay behind our own device-side adapter code.
- Linux input device paths, `evdev` details, and keyboard layout translation must stay inside the device adapter layer.
- Future player features must be added by clarifying their responsibility first, not by introducing generic `service` or `manager` folders.
- Media-pipeline work must follow `lofibox-zero-media-pipeline-spec.md` before format support is expanded in code.
- `TrackSource`, `Decoder`, `MetadataProvider`, `DspChain`, and `AudioSink` are valid responsibility boundaries when implementing the real playback stack.
- `TrackIdentityProvider`, `MetadataProvider`, `ArtworkProvider`, `LyricsProvider`, `AudioPlaybackBackend`, `ConnectivityProvider`, and `TagWriter` are valid responsibility boundaries for host runtime capability work.
- Shared app logic must not instantiate host-specific metadata readers, artwork readers, playback backends, or connectivity probes directly; those implementations must be injected from platform adapter code.
- Audio-DSP work must follow `lofibox-zero-audio-dsp-spec.md` before EQ surface area or runtime processing behavior is expanded.
- `EqProfile`, `EqEngine`, `EqManager`, `PresetRepository`, and `OutputDeviceBinding` are valid responsibility boundaries for mature EQ and DSP work.
- Streaming work must follow `lofibox-zero-streaming-spec.md` before code structure or page behavior is expanded.
- Protocol adapters, auth tokens, certificate handling, and share-specific details must stay behind source, catalog, resolution, metadata, and cache boundaries rather than leaking into page logic.
- If a later requirement changes the meaning of `core`, `app`, `platform/host`, `platform/device`, or container tooling, update this architecture document before changing code structure.
