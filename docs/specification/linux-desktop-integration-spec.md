<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Linux Desktop Integration Specification

## 1. Purpose

This document defines Linux desktop integration requirements for `LoFiBox Zero`.
Desktop integration is a product and packaging requirement, not decorative metadata.

LoFiBox's primary Linux desktop UI is a compact music application hosted in a
normal compositor- or window-manager-managed window. Linux desktop integration
must make it launchable, controllable, discoverable, and packageable without
forcing a traditional application menu bar into the product canvas.

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
Those translated commands must converge through the application command/query boundary defined in `application-command-boundary-spec.md` rather than directly calling UI pages, controllers, playback backends, remote providers, or runtime provider internals.

## 6. Product Shell Constraint

The Linux desktop target must support a compact no-menu-bar product canvas
inside a normal managed application window.

This means:

- no traditional application menu bar inside the primary product canvas
- no document-window assumptions in the UI model
- no protocol, settings, or help affordance hidden only behind a menu bar
- keyboard and page-local help affordances remain first-class
- desktop standards are used for launch, metadata, media control, notifications, and file/URL opening rather than for adding product chrome

The Wayland desktop target must use an `xdg_toplevel`, participate in ordinary
workspace management, and request server-side decorations when the compositor
supports `xdg-decoration`. The compositor owns titlebar dragging, minimizing,
and closing; LoFiBox must not emulate a desktop widget or claim a layer-shell
surface. X11 targets must likewise remain managed by the window manager and
must not use unmanaged-window behavior to avoid standard desktop actions.

The application icon must be installed through the hicolor icon theme under the desktop id `io.github.vicliu624.lofibox`, including the SVG metadata icon and a product-logo PNG fallback where available.

## 6.1 Pointer Projection Constraint

The desktop application is keyboard-first and keeps a compact product canvas.
Pointer input may be translated by the shell for development convenience, but the
pointer graphic is not product information and must not be projected into the
LoFiBox canvas.

The X11 presentation adapter therefore `MUST` define an invisible cursor for the
LoFiBox window. VNC, PocketFrame, and other screenshot harnesses used to validate
the 320x170 surface `MUST` hide the server/root cursor before capture. Public
documentation screenshots, release screenshots, and visual smoke evidence are
invalid if they contain the VNC/X11 cursor cross, pointer outline, or any other
capture-shell cursor artifact over the application surface.

## 7. Desktop Input Method Integration

The desktop application must participate in the user's Debian/Linux text-input session.

This means:

- text-entry pages receive committed UTF-8 text rather than raw ASCII-only key bytes
- in-progress input-method composition, when available, is represented as transient preedit projection rather than committed app state
- X11 text input should use XIM or an equivalent input-method context instead of plain `XLookupString`
- shell-level shortcuts such as `Super+H` are handled before text events enter the shared app router
- the Debian package must not hard-depend on one specific input-method framework such as IBus, Fcitx5, or uim merely to launch the player

The desktop integration layer may bridge system input-method output into app events.
It must not put Fcitx, IBus, XIM, or desktop-session protocol details into shared page, SearchState, or playback code.

Framebuffer/evdev device-profile input is governed by `lofibox-zero-text-input-spec.md` and `cardputer-zero-adaptation-spec.md`.
It must not be advertised as system CJK IME support unless a separate device input-method or input-proxy design is specified.

## 8. XDG Runtime Paths

Runtime data must follow:

- config: `~/.config/lofibox/`
- data: `~/.local/share/lofibox/`
- cache: `~/.cache/lofibox/`
- state: `~/.local/state/lofibox/`

If `HOME` is unset on Linux, the runtime path boundary may resolve `~` from the effective user's account record before falling back to temporary storage.
User data must never be written into `/usr`, `/opt`, installation directories, or the current working directory.

## 9. Current Implementation Convergence

As of 2026-04-27, runtime desktop integration has an explicit state boundary:

- desktop adapters report availability for MPRIS, D-Bus, media keys, and notifications
- files and URLs opened by the desktop environment become `DesktopOpenRequest` inputs before entering the unified playback/source model
- desktop commands are translated to app commands through `DesktopCommandAdapter`
- UI pages must not talk directly to D-Bus, notification backends, MIME handlers, or media-key listeners

Concrete backend adapters may evolve, but the handoff into the product must stay command/projection based.
As the application command boundary is introduced, desktop command adapters must use it as their product command target.
