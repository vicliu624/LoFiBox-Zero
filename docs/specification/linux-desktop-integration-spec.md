<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Linux Desktop Integration Specification

## 1. Purpose

This document defines Linux desktop integration requirements for `LoFiBox Zero`.
Desktop integration is a product and packaging requirement, not decorative metadata.

LoFiBox's primary UI is a chromeless desktop-widget music terminal.
Linux desktop integration must make it launchable, controllable, discoverable, and packageable without forcing a traditional application menu bar into the product surface.

## 2. Required Data Files

The project must provide:

- `data/io.github.vicliu624.lofibox.desktop`
- `data/io.github.vicliu624.lofibox.metainfo.xml`
- `data/io.github.vicliu624.lofibox.svg`
- MIME association files where needed

The installed paths must follow `debian-official-archive-spec.md`.

## 3. Desktop File Requirements

The `.desktop` file should support:

- application-menu visibility
- opening supported audio files
- opening supported URLs where appropriate
- categories including `Audio`, `Music`, and `Player`

The file must validate with `desktop-file-validate`.

## 4. AppStream Requirements

AppStream metadata must contain:

- `id`
- `name`
- `summary`
- `description`
- `project_license`
- `metadata_license`
- launchable desktop-id
- screenshots or explicitly governed screenshot placeholders
- content rating where required

The file must validate with `appstreamcli validate`.

## 5. Runtime Integration

The Linux desktop integration domain includes:

- MPRIS service
- D-Bus integration
- media-key handling
- desktop notifications
- MIME opening handoff into player commands

Desktop integration must translate external desktop events into core player commands.
It must not redefine core state or bypass playback, queue, or library boundaries.

## 6. Product Shell Constraint

The Linux desktop target must support a compact no-menu-bar product shell.

This means:

- no traditional application menu bar inside the primary product surface
- no document-window assumptions in the UI model
- no protocol, settings, or help affordance hidden only behind a menu bar
- keyboard and page-local help affordances remain first-class
- desktop standards are used for launch, metadata, media control, notifications, and file/URL opening rather than for adding product chrome

## 7. XDG Runtime Paths

Runtime data must follow:

- config: `~/.config/lofibox/`
- data: `~/.local/share/lofibox/`
- cache: `~/.cache/lofibox/`
- state: `~/.local/state/lofibox/`

User data must never be written into `/usr`, `/opt`, installation directories, or the current working directory.

## 9. Current Implementation Convergence

As of 2026-04-27, runtime desktop integration has an explicit state boundary:

- desktop adapters report availability for MPRIS, D-Bus, media keys, and notifications
- files and URLs opened by the desktop environment become `DesktopOpenRequest` inputs before entering the unified playback/source model
- desktop commands are translated to app commands through `DesktopCommandAdapter`
- UI pages must not talk directly to D-Bus, notification backends, MIME handlers, or media-key listeners

Concrete backend adapters may evolve, but the handoff into the product must stay command/projection based.
