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
- `application command boundary`
  Product command and query services that preserve product evolvability across GUI, desktop integration, future CLI, automation clients, and tests. It translates explicit product actions into playback, queue, library, source, remote, metadata, EQ, credential, cache, and diagnostic behavior without depending on page selection state or a particular runtime shell.
- `playback`
  Playback lifecycle, playback session, queue, seek, gapless, crossfade, and playback-mode semantics.
- `audio`
  Decoder, output, DSP, EQ, `ReplayGain`, sample-rate conversion, and audio-device responsibilities.
- `cache`
  Cache buckets, cache policy, remote directory cache, station cache, recent browse cache, offline audio, and offline sync planning.
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
- `text input`
  The boundary that receives committed UTF-8 text and optional preedit projection from runtime shells before editable product fields such as Search queries or source-profile fields mutate app state.
- `platform/host`
  Runtime capability adapters that expose one unified metadata, artwork, playback, logging, caching, enrichment, tag-writing, and connectivity surface to the shared app.
- `integrated product core`
  The cross-cutting product boundary that keeps media identity, media source, media stream, playback facts, audio pipeline, enrichment pipeline, library facts, UI projections, and runtime shells aligned.
- `platform/device`
  Linux integrations that exist for device-profile runtimes: framebuffer output, Linux input, and device-specific presentation.
- `device profile`
  A presentation and adapter profile for a specific Linux target form factor, such as `Cardputer Zero`; it may constrain screen size, chrome, input translation, and shell behavior, but it may not redefine product objects or playback semantics.
- `targets`
  Thin entry points that bind one platform runtime to the shared app.
- `cli`
  Future terminal parsing and formatting adapters. CLI code is not product command truth; it must dispatch into the application command boundary or into a runtime command client.
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
- Do not add new Windows compatibility branches for product runtime behavior without a prior specification change. The product target is Debian/Linux; cross-host convenience must not become architecture truth.

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
- Release runtime target: Debian/Linux. Host-machine facts from Windows development environments are not product facts and must not define new runtime behavior, UI contracts, packaging decisions, playback semantics, or remote-provider design.

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
- Shared app code may consume logical command keys and committed UTF-8 text events, but it must not call XIM, Fcitx, IBus, Linux `evdev`, or input-method daemon APIs directly.
- GUI, desktop integration, future CLI, automation clients, and tests must converge through the application command/query boundary defined in `application-command-boundary-spec.md`; they must not each drill into controllers, `AppRuntimeContext`, UI pages, or runtime providers independently.
- Page commands and product commands are distinct. Page-level confirm, selection, field-edit, and help behavior belongs to GUI routing. Product commands such as play, queue mutation, library scan, source profile mutation, remote browse, EQ update, credential mutation, cache command, and diagnostics belong behind application services.
- `AppRuntimeContext` is not a stable public command API. It may compose GUI runtime state, services, lifecycle, input routing, rendering, and page navigation, but product commands intended for GUI/desktop/CLI/automation/test reuse must move behind application services.
- `RuntimeServices` is a capability registry, not an application command surface. Command clients must not select metadata, playback, remote, cache, credential, or protocol providers directly.
- Future runtime commands that affect live playback, active queue, current output, or in-memory app state must target the running process through a runtime command client/server design instead of launching a second independent runtime or mutating persistence behind the app's back.
- X11 presentation adapters own desktop input-method integration and must convert system IME output into committed UTF-8 text plus optional preedit projection before it reaches app state.
- Device-profile `evdev` adapters own direct Linux `EV_KEY` translation. They may emit directly translatable committed text, but they must not claim system CJK input-method support without a separate device input-method specification.
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
- Integrated product core responsibilities must follow `integrated-product-core-spec.md`.
- Track identity and fingerprint-backed enrichment responsibilities must follow `lofibox-zero-track-identity-spec.md`.
- Shared cross-page application-state responsibilities must follow `lofibox-zero-app-state-spec.md`.
- Text input, committed UTF-8, preedit, and system input-method responsibilities must follow `lofibox-zero-text-input-spec.md`.
- Product command/query responsibilities shared by GUI, desktop integration, future CLI, automation clients, and tests must follow `application-command-boundary-spec.md`.
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
- `src/app/app_command_executor.*` owns page command execution semantics: main-menu selection meanings, Settings row actions, list confirmation behavior, playback command forwarding, playback-mode cycling, and EQ selection/gain limits; `LoFiBoxApp` must not directly implement those branches or constants.
- `src/app/app_page_model.*` owns page titles, current page rows, Settings rows, browse-list classification, and list viewport limits; `LoFiBoxApp` must not directly depend on `ui/ui_theme.h`, `ui/ui_primitives.h`, or define page-model branches itself.
- `src/app/app_renderer.*` owns page-level render routing and UI view-model assembly; `LoFiBoxApp` delegates `render()` through a narrow render target interface and must not directly include `ui/pages/*`.
- `src/app/app_lifecycle.*` owns application tick ordering: runtime status refresh, media-library loading, boot-page transition, and playback update; `LoFiBoxApp` delegates `update()` through a narrow lifecycle target interface.
- `src/app/app_runtime_context.*` owns runtime state, runtime services, controllers, and the target-interface implementations used by input routing, render routing, lifecycle, and command execution; `LoFiBoxApp` must remain a thin public facade and must not directly include app state, controllers, page model, render router, input router, lifecycle, or command executor headers.
- `src/app/app_runtime_context.*` is a current GUI runtime composition shell, not the long-term owner of reusable product command semantics. As application services are introduced, product commands must move out of page-specific `AppRuntimeContext` methods and into the application command/query boundary.
- Future `src/application` code owns application command/query services and the service registry described in `application-command-boundary-spec.md`. It must not include concrete platform adapters, UI pages, or target entry points.
- Future `src/cli` code owns terminal argument parsing, dispatch, and text/JSON output formatting only. It must depend on application services or a runtime command client, not on `AppRuntimeContext`, UI pages, controllers, or provider implementations.
- `src/app/app_runtime_state.*` owns scalar/session runtime state; `src/app/app_controller_set.*` owns app controllers; `AppRuntimeContext` may adapt these objects to target interfaces but must not become a new raw-field aggregate.
- `src/app/runtime_services.*` owns the runtime capability registry; runtime services must be grouped as connectivity, metadata, playback, remote, and cache capabilities instead of exposing top-level provider fields.
- `src/playback/playback_controller.*` owns playback state-machine, queue, playback-mode, and current-track command semantics; it must not own enrichment worker threads, pending result maps, or network enrichment merging.
- `src/playback/playback_enrichment_coordinator.*` owns asynchronous enrichment request scheduling, generation guards, pending-result merge, and application of metadata/artwork/lyrics enrichment to library facts.
- `src/platform/host/acoustid_client.cpp`, `musicbrainz_client.cpp`, `cover_art_archive_client.cpp`, `lyrics_source_clients.cpp`, and `ffprobe_metadata_reader.cpp` own concrete enrichment protocol clients. `runtime_enrichment_clients.cpp` may provide shared helpers/orchestration but must not regain concrete protocol method ownership.
- `src/platform/host/lyrics_pipeline_components.*` owns lyrics cache and writeback policy. `LyricsProvider` may compose embedded lyrics reading, online resolving, cache use, match guarding, and writeback decisions but must not hide all of those responsibilities in one provider body.
- `src/app/library_query_service.*` owns library fact queries. `src/app/library_navigation_service.*` owns library list title/row projection. `src/app/library_open_action_resolver.*` owns list-open action semantics. `src/app/library_list_context.h` owns browse/list context state.
- `src/app/app_projection_builder.*` owns app-state to UI projection assembly. `AppRenderer` owns dispatch and page invocation, not projection translation.
- `src/ui/widgets/lyrics_layout.*` owns lyric line parsing and active-line/window selection. `src/ui/effects/lyrics_spectrum_effect.*` owns lyrics-page visual spectrum algorithms.
- `src/platform/host/runtime_host_tools.*` owns named host utility sub-boundaries for text parsing, JSON helpers, cache path derivation, and helper script resolution.
- `src/playback`, `src/audio`, `src/cache`, `src/metadata`, `src/library`, `src/playlist`, `src/remote`, `src/plugins`, `src/desktop`, `src/platform`, `src/security`, and `src/ui` are the target semantic structure for long-term source ownership.
- A semantic source directory must not be an empty aspiration. If a directory exists under `src`, it must contain its implementation contract or implementation files, and `scripts/check-implementation-placement.ps1` must reject hidden implementation elsewhere.
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
- X11 input-method contexts, Fcitx/IBus session protocols, and `evdev` key translation must stay inside runtime shell adapters; shared Search, Settings, and page code may only see logical commands, committed UTF-8 text, and optional preedit projection.
- Future player features must be added by clarifying their responsibility first, not by introducing generic `service` or `manager` folders.
- Do not put asynchronous enrichment, metadata/artwork/lyrics network calls, or pending-result merge logic back into `PlaybackController`.
- Do not collapse concrete enrichment protocol clients back into `runtime_enrichment_clients.cpp`.
- Do not let `LyricsProvider` decide source trust, cache, and writeback semantics without explicit pipeline components.
- Do not let `LibraryController` become both scan/query truth, page-navigation projection owner, and list-open action resolver.
- Do not add external command surfaces by calling `AppRuntimeContext` page methods, simulating selected rows, or reusing GUI field-edit flows.
- Do not make `targets` or `src/cli` own product command semantics; they may only parse, dispatch, and format.
- Do not expose controllers or `RuntimeServices` provider groups as the stable external command boundary when an application service should own the product command.
- Do not let `AppRenderer` become a projection-builder god object; view assembly belongs in `AppProjectionBuilder`.
- Do not put lyric parsing, active-line timing, scrolling-window selection, or spectrum rendering algorithms back into `LyricsPage`.
- Do not use `runtime_host_internal` as a junk drawer for unrelated host helpers; each helper family must have a named boundary.
- Media-pipeline work must follow `lofibox-zero-media-pipeline-spec.md` before format support is expanded in code.
- `TrackSource`, `Decoder`, `MetadataProvider`, `DspChain`, and `AudioSink` are valid responsibility boundaries when implementing the real playback stack.
- `TrackIdentityProvider`, `MetadataProvider`, `ArtworkProvider`, `LyricsProvider`, `AudioPlaybackBackend`, `ConnectivityProvider`, and `TagWriter` are valid responsibility boundaries for host runtime capability work.
- `RuntimeServiceRegistry`, `ConnectivityServices`, `MetadataServices`, `PlaybackServices`, `RemoteMediaServices`, and `CacheServices` are valid runtime injection boundaries; new providers must join the correct capability group instead of adding flat global fields.
- Shared app logic must not instantiate host-specific metadata readers, artwork readers, playback backends, or connectivity probes directly; those implementations must be injected from platform adapter code.
- Audio-DSP work must follow `lofibox-zero-audio-dsp-spec.md` before EQ surface area or runtime processing behavior is expanded.
- `EqProfile`, `EqEngine`, `EqManager`, `PresetRepository`, and `OutputDeviceBinding` are valid responsibility boundaries for mature EQ and DSP work.
- Streaming work must follow `lofibox-zero-streaming-spec.md` before code structure or page behavior is expanded.
- Protocol adapters, auth tokens, certificate handling, and share-specific details must stay behind source, catalog, resolution, metadata, and cache boundaries rather than leaking into page logic.
- If a later requirement changes the meaning of `core`, `app`, `platform/host`, `platform/device`, or container tooling, update this architecture document before changing code structure.
