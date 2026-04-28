<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Final Product Specification

## 1. Purpose

This document defines the final-form product truth for `LoFiBox Zero`.

It exists to prevent implementation progress documents from silently becoming the product definition.
This file answers:

- what `LoFiBox Zero` fundamentally is
- what capabilities the finished product must contain
- which product objects and user flows must survive implementation changes
- which other specifications are interface or implementation documents rather than product truth

If another document describes the current implementation and this document describes the final product, this document wins on product meaning.

## 2. Authority

This is the authoritative product-shape document for `LoFiBox Zero`.

Other specification types serve different roles:

- `project-architecture-spec.md`
  Enduring architecture, layering, and ownership rules
- `debian-official-archive-spec.md`
  Debian and Ubuntu official-archive engineering constraints
- `dependency-policy-spec.md`
  Dependency admission, build-time network, vendoring, and archive-compatibility rules
- `plugin-provider-system-spec.md`
  Provider and plugin boundaries for remote-source and protocol capabilities
- `linux-desktop-integration-spec.md`
  Linux desktop integration, XDG, desktop file, AppStream, D-Bus, MPRIS, media-key, and notification rules
- `cardputer-zero-adaptation-spec.md`
  Cardputer Zero small-screen, chromeless, keyboard-first adaptation profile rules
- `testing-ci-spec.md`
  Unit, integration, protocol, packaging, CI, and autopkgtest requirements
- `copyright-resource-governance-spec.md`
  Source, asset, fixture, screenshot, third-party, and Debian copyright rules
- `lofibox-zero-app-state-spec.md`
  Shared cross-page application-state contracts
- `lofibox-zero-persistence-spec.md`
  Persistence domains, load order, and repair rules
- `lofibox-zero-credential-spec.md`
  Credential references, secret storage, and session-handling rules
- `lofibox-zero-library-indexing-spec.md`
  Library scan, index build, refresh, and rebuild rules
- `lofibox-zero-media-pipeline-spec.md`
  Media decode, metadata, DSP-handoff, and output-path contracts
- `lofibox-zero-track-identity-spec.md`
  Audio fingerprinting, recording identity, and enrichment authority contracts
- `lofibox-zero-audio-dsp-spec.md`
  EQ and DSP-domain contracts
- `lofibox-zero-streaming-spec.md`
  Remote-source and media-server capability contracts
- `lofibox-zero-text-input-spec.md`
  Committed text, preedit, Unicode editing, and system input-method boundary contracts
- `lofibox-zero-page-spec.md`
  Current UI implementation contract, not final product truth
- `lofibox-zero-layout-spec.md`
  Current UI layout contract
- `lofibox-zero-visual-design-spec.md`
  Current UI visual contract
- `implementation-status.md`
  Current progress only

## 3. Product Definition

`LoFiBox Zero` is a pure C++ Linux desktop music player whose long-term distribution target is Debian and Ubuntu official repositories.

The finished product must be capable of being installed through official distribution channels, with the intended user-facing endpoint:

```sh
sudo apt install lofibox
```

The primary product presentation is a chromeless desktop-widget music terminal.
It must look and behave more like a focused desktop gadget than a conventional document-style desktop application.

The main product surface must not expose a traditional application menu bar.
Desktop integration still exists through `.desktop`, AppStream, MIME, MPRIS, D-Bus, media keys, notifications, and XDG paths, but those integrations must not force conventional desktop chrome into the product UI.

`Cardputer Zero`, `PocketFrame`, framebuffer builds, VNC validation, and other device-shaped runtimes are adapter profiles and validation targets.
They are important, but they are not the product's highest identity.

The current active hardware-profile target includes `Cardputer Zero`.
That profile is first-class for adaptation work even though it does not redefine the whole product.
Its screen and interaction form naturally fit the product's chromeless appliance-style surface.

Its final form is:

- offline-first
- browse-first
- Linux-native
- desktop-integrated
- packageable for official Debian and Ubuntu archive review
- operable by keyboard and small-screen profiles where those profiles are active
- presented as a chromeless desktop-widget style app without a traditional menu bar
- metadata-aware
- recording-identity-aware
- cover-art-aware
- queue-based
- DSP-capable
- able to unify local media and selected remote media sources under one player model

It is not:

- a cloud-first media service
- a board-specific demo shell
- a conventional document-style desktop application with a persistent menu bar
- a protocol-specific media tool
- a packaging workaround that removes product capabilities to make review easier
- a simulator-specific implementation

## 4. Product Identity And Non-Goals

### 4.1 Identity

The finished product must remain:

- a focused music player
- coherent as a Linux desktop application
- chromeless and widget-like in its primary surface
- usable through keyboard-first and device-profile surfaces
- consistent across local, remote, and streamed content where their semantics overlap
- suitable for Debian and Ubuntu packaging norms without deleting product domains

### 4.2 Non-Goals

The finished product must not drift into:

- account-centric cloud dependency
- generic system-administration UI
- plugin-first user experience
- protocol-shaped user experience
- decorative chrome that weakens playback usability
- app-internal dependency downloading or binary-vendor packaging shortcuts

## 5. Canonical Product Objects

The finished product must recognize these as real product objects:

- `Track`
- `Album`
- `Artist`
- `Genre`
- `Composer`
- `Playlist`
- `Queue`
- `Station`
- `PlayableItem`
- `PlaybackSession`
- `EqProfile`
- `Source`
- `TrackIdentity`

These objects may be backed by different local or remote implementations, but the product must not let transport details redefine them.

## 6. Final Capability Domains

The finished product must include these capability domains.

### 6.1 Library And Catalog

- local library scan and browse
- incremental scan and index rebuild behavior
- media-library database and migration strategy
- album, artist, song, genre, composer, and compilation navigation
- smart playlists derived from listening history
- metadata and cover-art-aware presentation

### 6.2 Playback And Queue

- shared playback queue
- playback state that survives page changes
- previous, next, pause, play, shuffle, repeat, progress, and queue continuity
- seek, queue mutation, playlist playback, and playback-mode semantics
- one coherent `Now Playing` experience

### 6.3 Source Model

- local filesystem media
- LAN or share-backed media roots, including `SMB`, `NFS`, `WebDAV`, `FTP`, and `SFTP`
- remote playable URLs
- internet radio and station entries
- media-server-backed sources, including `Jellyfin`, `Navidrome`, `Emby`, `Subsonic`, `OpenSubsonic`, `DLNA`, and `UPnP`
- saved source management rather than ad-hoc one-off fields

### 6.4 Streaming And Remote Media

- direct remote audio playback
- playlist manifest entry points
- radio or station-style playback
- media-server integration
- unified search and browse where the source semantics support it

### 6.5 Media Pipeline

- format recognition
- decode through one shared pipeline model
- common audio formats such as `MP3`, `FLAC`, `OGG`, `WAV`, `AAC`, and `M4A`
- normalized metadata extraction
- `ID3`, `Vorbis Comment`, `FLAC` tag, and `MP4` metadata handling
- fingerprint-capable track identity resolution
- DSP insertion after decode
- output through a stable sink abstraction

### 6.6 EQ And DSP

- first-class equalizer experience
- multi-band EQ, biquad filters, `ReplayGain`, volume control, and output-device selection
- preset-capable DSP model
- processing-chain semantics rather than page-local slider semantics
- one shared DSP entry path for local and remote content where behavior allows it

### 6.7 Settings And Device Awareness

- a restrained settings surface
- truthful device and product information
- persistent user choices
- output-aware behavior where the product supports multiple output paths

### 6.8 Linux Desktop Integration

- `.desktop` file
- AppStream metadata
- hicolor application icon
- MIME association for audio files and URLs where appropriate
- MPRIS and D-Bus integration
- media-key handling
- desktop notifications
- XDG-compliant config, data, cache, and state paths

Linux desktop integration must not imply a conventional menu bar or document-window user experience.
The product may be launchable and controllable through desktop standards while still presenting a compact chromeless widget surface.

### 6.9 Device Profiles

The product may expose different presentation profiles over the same product model.

The `Cardputer Zero` profile is a current required profile and must preserve:

- `320x170` small-screen layout assumptions where the target display uses that logical resolution
- hardware-keyboard-first navigation
- the same no-menu-bar, chromeless product surface as the primary app
- no additional desktop chrome presented as part of the app UI
- page-local `F1:HELP` style affordances where the small-screen profile needs discoverability
- the same playback, library, metadata, remote-source, DSP, persistence, and credential semantics as other Linux targets

Device profiles are presentation and adapter contracts.
They must not fork the product's core media model, playback model, source model, metadata model, or DSP model.

## 7. Final Interaction Model

The final product must remain keyboard-capable and device-profile-aware.

Core interaction invariants:

- every essential path is operable without pointer input
- navigation is predictable and page-consistent
- editable text accepts committed Unicode text through the active runtime shell without making the player own general input-method engines
- playback adjustments feel realtime
- source changes, EQ changes, and queue changes do not require the user to reason about backend internals

## 8. Final Surface Model

The finished product must expose the following first-class user-facing surfaces, whether as full pages or tightly integrated primary subviews:

- startup or readiness surface
- home or main navigation surface
- library browse surface
- search surface
- playlists surface
- queue or up-next surface
- now playing surface
- source management surface
- remote-source browse surface when streaming or servers are enabled
- equalizer or DSP surface
- settings surface
- about or diagnostics surface
- Linux desktop integration surfaces where exposed by the platform
- device-profile surfaces that adapt the shared chromeless shell to the target form factor
- desktop-widget surfaces that preserve the no-menu-bar product identity

This document defines that these surfaces must exist in the final product model.
Their current implementation shape belongs to the UI implementation specifications.

## 9. Final Product Invariants

The following must remain true even as implementation evolves:

- local playback remains first-class and does not require network access
- local and remote content share one player model where their semantics truly overlap
- streaming does not fork queue, metadata, or DSP into a separate product
- EQ and DSP are part of the audio-processing chain, not just one page
- Debian and Ubuntu official-archive readiness is an engineering constraint, not a reason to delete feature domains
- build, install, runtime paths, dependency handling, copyright handling, and tests must converge toward distribution norms
- page-level implementation constraints must never become the product's highest truth
- implementation progress must be described separately from final product meaning

## 10. Progress Separation Rule

`LoFiBox Zero` must keep these categories separate:

- final product truth
- enduring architecture and interface contracts
- current UI implementation constraints
- current delivery or progress status

No document describing current progress may redefine the finished product merely by existing earlier in the repository.
