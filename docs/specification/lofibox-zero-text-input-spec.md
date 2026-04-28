<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Text Input And Input Method Specification

## 1. Purpose

This document defines how `LoFiBox Zero` receives editable text.

It exists to keep CJK input, system input-method integration, raw device keys, and app search text from collapsing into one ambiguous feature.

This document applies to:

- Search query input
- remote profile field editing
- future playlist or metadata text fields
- runtime shell input adapters that translate platform text input into app events

## 2. Related Specs

- Product surface truth: `docs/specification/lofibox-zero-final-product-spec.md`
- Architecture boundaries: `docs/specification/project-architecture-spec.md`
- Linux desktop shell behavior: `docs/specification/linux-desktop-integration-spec.md`
- Shared app-state rules: `docs/specification/lofibox-zero-app-state-spec.md`
- Search state: `docs/specification/app-states/search-state.md`
- Current Search page: `docs/specification/current-ui-pages/search.md`
- Remote profile settings page: `docs/specification/current-ui-pages/remote-profile-settings.md`
- Remote field editor page: `docs/specification/current-ui-pages/remote-field-editor.md`

## 3. Core Distinctions

### 3.1 Query Text Versus Input Method Composition

- `committed text` is finalized Unicode text delivered to the app by the active input path.
- `preedit text` is in-progress composition owned by an input method or text-input adapter.
- `raw key events` are physical or logical key presses before text composition.

Only committed text may mutate product text fields such as `SearchState.query`.
Preedit text may be projected in UI while composition is active, but it is not durable query truth and must not trigger search results as if it were committed.

### 3.2 App Text Fields Versus Input Method Engines

LoFiBox owns:

- which page currently accepts text
- the committed UTF-8 text stored in app state
- cursor/editing behavior required by the page
- Unicode-safe deletion and truncation
- search or field-save behavior after committed text changes

LoFiBox does not own:

- Pinyin, Zhuyin, Wubi, Kana, Romaji, Hangul, Rime, Mozc, Anthy, Chewing, libpinyin, or other general input-method engines
- CJK dictionaries or candidate ranking
- system input-method selection
- desktop input-method daemon lifecycle

## 4. Debian/Linux Input Method Boundary

On Debian/Linux desktop sessions, CJK input is a system and user-session concern.
Users may configure input method frameworks such as IBus, Fcitx5, or uim through the desktop session and Debian input-method tooling.

LoFiBox must integrate with that session-level input method and consume committed UTF-8 text.
LoFiBox must not vendor or implement a general-purpose CJK input method as part of the music player.

The Debian package must not require one specific input-method framework to launch.
Input-method frameworks and language engines may be documented or recommended for CJK text entry, but they are not mandatory runtime dependencies of the player core.

## 5. Runtime Shell Responsibilities

### 5.1 Shared App Layer

The shared app layer consumes:

- logical command keys such as `F1`, `BACKSPACE`, `ENTER`, `UP`, `DOWN`
- committed UTF-8 text events for text-entry pages
- optional transient preedit projection data when a platform adapter can provide it

The shared app layer must not include Linux input headers, X11 input-method calls, Fcitx/IBus protocol clients, or device-path knowledge.

### 5.2 X11 Desktop-Widget Target

The X11 shell is the primary Debian desktop presentation target.
It must integrate with the system input method for text-entry pages.

A correct X11 path must:

- use XIM or an equivalent toolkit/input-method context rather than ASCII-only `XLookupString`
- emit committed UTF-8 text events to the shared app
- keep preedit text separate from committed query text
- preserve shell-level shortcuts such as `Super+H` before forwarding text or command events

### 5.3 Framebuffer/Evdev Device Target

The framebuffer/evdev target reads Linux `EV_KEY` events directly.
It may translate physical keys into app command keys and direct printable characters.

It must not claim system CJK input-method support merely because the desktop package supports it.
If a framebuffer device profile needs CJK composition, that work must be specified as a separate device input method, input proxy, or alternate shell decision before implementation.

Until such a device-profile decision exists, framebuffer/evdev text input is limited to directly translatable key text.

## 6. Unicode Requirements

All committed text entering app state must be valid UTF-8.

Text fields must:

- reject or repair invalid UTF-8 before storing it
- delete by Unicode code point or grapheme cluster, not by raw byte
- truncate by display-safe or Unicode-safe boundaries, not by raw byte
- render CJK-capable text through the normal font stack
- keep password/secret fields redacted according to credential rules even when the input is Unicode

Search matching should use a normalized comparison layer.
The minimum acceptable behavior is preserving exact CJK substring matching without corrupting UTF-8.
More advanced folding, transliteration, kana width normalization, or locale-specific ranking belongs to search governance and must not be confused with input-method composition.

## 7. Text-Entry Page Contract

A page that owns text entry must define:

- which state field receives committed text
- whether preedit text is displayed
- how `BACKSPACE` edits text
- how `DELETE` or a page-local clear action behaves
- what `ENTER`/`OK` commits or activates
- whether global playback shortcuts remain active while editing
- how invalid UTF-8 or unsupported platform text input is surfaced
- whether the text is ordinary profile text, search query text, or secret material governed by credential rules

Raw printable keys are allowed as a fallback only when they are already committed text.
They must not be treated as an input-method candidate selection protocol.

## 8. Test Requirements

The test suite must cover:

- appending committed UTF-8 text to Search
- Unicode-safe backspace for multibyte text
- invalid UTF-8 rejection or repair
- X11 text adapter behavior for committed UTF-8
- framebuffer/evdev capability boundaries so ASCII-only direct input is not advertised as CJK IME support
- Search matching over local and remote metadata containing non-ASCII text

## 9. Non-Goals

- Implementing a general CJK input method inside LoFiBox
- Owning CJK dictionaries, candidate windows, or conversion engines in the shared app
- Making one specific Debian input-method framework a hard dependency of the music player
- Treating framebuffer/evdev raw key input as equivalent to desktop input-method support
