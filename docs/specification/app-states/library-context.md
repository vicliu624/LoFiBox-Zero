<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero App State Contract: LibraryContext

## 1. Purpose

- Own the cross-page browse context that determines filtered album and song views.

## 2. Related Specs

- Shared app-state rules: `docs/specification/lofibox-zero-app-state-spec.md`
- Current browse pages: `docs/specification/current-ui-pages/albums.md`, `docs/specification/current-ui-pages/songs.md`, `docs/specification/current-ui-pages/genres.md`, `docs/specification/current-ui-pages/composers.md`, `docs/specification/current-ui-pages/compilations.md`

## 3. Owned Truth

- current browse-filter context
- current context payload such as selected artist, album, genre, composer, or compilation album
- title-driving context semantics for filtered pages

## 4. Data Dependencies

- selected entity from the parent browse page
- entity display name or label
- current navigation origin

## 5. State Enumeration

- `STATE_LIBRARY_CONTEXT_NONE`
  no filtered library context is active
- `STATE_LIBRARY_CONTEXT_ARTIST`
  current context is artist-filtered
- `STATE_LIBRARY_CONTEXT_ALBUM`
  current context is album-filtered
- `STATE_LIBRARY_CONTEXT_GENRE`
  current context is genre-filtered
- `STATE_LIBRARY_CONTEXT_COMPOSER`
  current context is composer-filtered
- `STATE_LIBRARY_CONTEXT_COMPILATION`
  current context is compilation-filtered

## 6. Invariants

- Entering `Albums` from `Artists` establishes artist-filter context.
- Entering `Songs` from `Albums`, `Genres`, `Composers`, or `Compilations` establishes the appropriate song-list context.
- Page titles that depend on context must reflect this shared context truth rather than inventing local labels.

## 7. Event Contract

- `EVT_CLEAR_CONTEXT`
  - effect: remove active filtered context
- `EVT_ENTER_ARTIST_CONTEXT`
  - effect: set selected artist as active context
- `EVT_ENTER_ALBUM_CONTEXT`
  - effect: set selected album as active context
- `EVT_ENTER_GENRE_CONTEXT`
  - effect: set selected genre as active context
- `EVT_ENTER_COMPOSER_CONTEXT`
  - effect: set selected composer as active context
- `EVT_ENTER_COMPILATION_CONTEXT`
  - effect: set selected compilation album as active context

## 8. Transition Contract

- `STATE_LIBRARY_CONTEXT_NONE + EVT_ENTER_ARTIST_CONTEXT` -> effect: store selected artist -> `STATE_LIBRARY_CONTEXT_ARTIST`
- `STATE_LIBRARY_CONTEXT_NONE + EVT_ENTER_ALBUM_CONTEXT` -> effect: store selected album -> `STATE_LIBRARY_CONTEXT_ALBUM`
- `STATE_LIBRARY_CONTEXT_NONE + EVT_ENTER_GENRE_CONTEXT` -> effect: store selected genre -> `STATE_LIBRARY_CONTEXT_GENRE`
- `STATE_LIBRARY_CONTEXT_NONE + EVT_ENTER_COMPOSER_CONTEXT` -> effect: store selected composer -> `STATE_LIBRARY_CONTEXT_COMPOSER`
- `STATE_LIBRARY_CONTEXT_NONE + EVT_ENTER_COMPILATION_CONTEXT` -> effect: store selected compilation -> `STATE_LIBRARY_CONTEXT_COMPILATION`
- `STATE_LIBRARY_CONTEXT_ARTIST + EVT_CLEAR_CONTEXT` -> effect: clear stored context -> `STATE_LIBRARY_CONTEXT_NONE`
- `STATE_LIBRARY_CONTEXT_ALBUM + EVT_CLEAR_CONTEXT` -> effect: clear stored context -> `STATE_LIBRARY_CONTEXT_NONE`
- `STATE_LIBRARY_CONTEXT_GENRE + EVT_CLEAR_CONTEXT` -> effect: clear stored context -> `STATE_LIBRARY_CONTEXT_NONE`
- `STATE_LIBRARY_CONTEXT_COMPOSER + EVT_CLEAR_CONTEXT` -> effect: clear stored context -> `STATE_LIBRARY_CONTEXT_NONE`
- `STATE_LIBRARY_CONTEXT_COMPILATION + EVT_CLEAR_CONTEXT` -> effect: clear stored context -> `STATE_LIBRARY_CONTEXT_NONE`

## 9. Default State

- Default library context is `STATE_LIBRARY_CONTEXT_NONE`.

## 10. Error State

- Invalid context payloads `MUST NOT` create fake browse labels or mismatched page titles.
- If a filtered context cannot be established truthfully, the contract must degrade to `STATE_LIBRARY_CONTEXT_NONE`.

## 11. Non-Goals

- Sort-mode state
- Persistent cross-session browse-history restoration
