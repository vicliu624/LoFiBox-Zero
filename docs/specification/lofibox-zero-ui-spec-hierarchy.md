<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero UI Specification Hierarchy

## 1. Purpose

This document defines which specification controls UI implementation decisions in `LoFiBox Zero`.

Without this hierarchy, future UI work will drift because product intent, page behavior, styling, and geometry will all compete at the same level.

This document answers the practical engineering question:

`When writing UI code, which specification is the source of truth?`

## 2. The Short Answer

When writing UI code, use these specifications in this order:

1. `lofibox-zero-final-product-spec.md`
2. `debian-official-archive-spec.md`
3. `project-architecture-spec.md`
4. `dependency-policy-spec.md`
5. `plugin-provider-system-spec.md`
6. `linux-desktop-integration-spec.md`
7. `cardputer-zero-adaptation-spec.md`
8. `copyright-resource-governance-spec.md`
9. `testing-ci-spec.md`
10. `lofibox-zero-persistence-spec.md` for save, load, default, and repair behavior
11. `lofibox-zero-credential-spec.md` for secret references, secure storage, and session rules
12. `lofibox-zero-library-indexing-spec.md` for scan, index build, refresh, rebuild, and library readiness rules
13. `lofibox-zero-app-state-spec.md` for shared playback, navigation, library-context, library-index, queue, search, source, credential, network, and settings-state work
14. `lofibox-zero-media-pipeline-spec.md` for decode, playback-pipeline, and output-path capability work
15. `lofibox-zero-track-identity-spec.md` for fingerprint-backed recording identity and enrichment authority
16. `lofibox-zero-audio-dsp-spec.md` for EQ and DSP capability work
17. `lofibox-zero-streaming-spec.md` for streaming-related capability work
18. `lofibox-zero-text-input-spec.md` for committed text, preedit, Unicode editing, and system input-method boundaries
19. `lofibox-zero-page-spec.md`
20. `lofibox-zero-layout-spec.md`
21. `lofibox-zero-visual-design-spec.md`
22. `legacy-lofibox-product-guidance.md`

They do not all answer the same question.

## 3. What Each Document Controls

### 3.1 `lofibox-zero-final-product-spec.md`

This document controls:

- what the finished product fundamentally is
- which first-class surfaces and capability domains must exist
- which implementation documents are current-progress contracts rather than product truth

When writing UI code, ask:

- am I implementing final product meaning or merely copying today's progress snapshot?
- is this decision about the product itself, or only about the current implementation?

If the final product spec says a capability or surface belongs in the finished product, a current implementation shortcut may not redefine it away.

### 3.2 `project-architecture-spec.md`

This document controls:

- where code is allowed to live
- what belongs to `core`, `app`, `platform/host`, and `platform/device`
- which concepts are shared truth versus adapter detail

When writing UI code, ask:

- does this belong in shared app code or in a Linux adapter?
- am I leaking Linux device details into shared UI logic?

If `project-architecture-spec.md` says a design idea belongs in the wrong layer, you do not implement it there.

### 3.3 Distribution And Governance Specs

These documents control whether UI and feature work remains compatible with the larger Linux distribution target:

- `debian-official-archive-spec.md`
- `dependency-policy-spec.md`
- `plugin-provider-system-spec.md`
- `linux-desktop-integration-spec.md`
- `cardputer-zero-adaptation-spec.md`
- `copyright-resource-governance-spec.md`
- `testing-ci-spec.md`

When writing UI or feature code, ask:

- am I adding a dependency, asset, runtime path, protocol provider, or desktop behavior that affects packaging?
- am I solving complexity by deleting a product capability rather than organizing it behind a proper boundary?
- am I leaking desktop, protocol, or packaging details into product state?
- am I confusing a required device profile, such as `Cardputer Zero`, with the product's whole identity?
- am I accidentally turning the chromeless desktop-widget shell into a conventional menu-bar application?

If a feature requires new dependencies, install paths, provider split, desktop files, tests, or copyright review, the relevant governance spec must be updated before implementation shortcuts become structure.

### 3.4 `lofibox-zero-persistence-spec.md`

This document controls:

- what is persisted versus runtime-only
- when persisted domains load
- how invalid persisted state is repaired or degraded safely

When writing code or UI-adjacent logic, ask:

- should this truth survive restart or not?
- where do defaults come from?
- what happens when saved data is invalid?

If the persistence spec says a domain is runtime-only, do not silently persist it just because it seems convenient.

### 3.5 `lofibox-zero-credential-spec.md`

This document controls:

- what `credential_ref` really means
- what may persist versus what must stay in a secure store
- how sessions differ from durable credentials

When writing code or UI-adjacent logic, ask:

- am I persisting raw secret material where only a reference should persist?
- is this durable credential truth or runtime session truth?
- does this behavior belong to credential lifecycle rather than general persistence?

If the credential spec says secret material must stay outside ordinary records, do not smuggle it into source profiles or settings blobs.

### 3.6 `lofibox-zero-library-indexing-spec.md`

This document controls:

- what counts as an indexable root
- how the library is scanned and indexed
- when refresh or rebuild semantics apply

When writing code or UI-adjacent logic, ask:

- is this local-library indexing behavior or some other capability?
- when does browsing become truthfully ready?
- when is incremental refresh enough versus a full rebuild?

If the indexing spec says library readiness is not complete, pages must not pretend indexed browse data is fully available.

### 3.7 `lofibox-zero-app-state-spec.md`

This document controls:

- which truths are shared across pages rather than owned by one page
- how playback, navigation, library context, queue, search, source, and settings state are modeled
- how pages collaborate through state machines instead of hidden coupling

When writing UI code, ask:

- is this truth page-local or app-global?
- am I duplicating playback, back-stack, library-context, queue, search, source, or settings state inside a page file?
- should this page trigger a shared state event instead of owning the transition itself?

If the app-state spec says a truth is global, the page may project it but must not redefine it.

### 3.8 `lofibox-zero-media-pipeline-spec.md`

This document controls:

- what a playable source becomes after resolution
- what belongs to decode, metadata, DSP handoff, and output
- how local and remote track-like media converge through one playback path

When writing UI code for playback-adjacent features, ask:

- am I leaking backend or parser details into shared UI logic?
- is this a decode or output capability fact rather than a page concept?
- am I skipping the shared pipeline model because one early source is convenient?

If the media-pipeline spec says a behavior belongs behind pipeline boundaries, the UI must not invent ad-hoc source-specific logic.

### 3.9 `lofibox-zero-track-identity-spec.md`

This document controls:

- whether a metadata-like fact is actually recording identity
- how audio fingerprints, MusicBrainz identifiers, confidence, and source authority are interpreted
- which providers may consume identity without redefining it
- when identity facts may be written back to media files

When writing identity-adjacent UI code, ask:

- am I showing or editing metadata, or am I exposing recording identity?
- am I treating a filename or lyrics hit as proof of identity?
- am I leaking provider-specific matching details into page logic?

If the track-identity spec says a fact belongs behind identity resolution, the UI must not recreate its own matching rules.

### 3.10 `lofibox-zero-audio-dsp-spec.md`

This document controls:

- whether an EQ or DSP concept belongs in the product at all
- what belongs inside `DspChain`
- how presets, runtime state, safety controls, and device bindings are allowed to work

When writing EQ or DSP-related UI code, ask:

- is this a profile concept, a session-state concept, or a processing-engine concept?
- am I confusing the current implemented EQ page with the long-term DSP architecture?
- am I introducing page controls for DSP behavior that the current page spec does not actually allow yet?

If the audio-DSP spec says a behavior belongs to the processing chain rather than the page shell, the UI must not flatten it into page-local special cases.

### 3.11 `lofibox-zero-streaming-spec.md`

This document controls:

- whether a remote-media concept belongs in the product at all
- which streaming responsibility owns it
- how local and remote media are allowed to unify
- what metadata, cache, auth, and playback distinctions must remain explicit

When writing streaming-related UI code, ask:

- is this a source-management concept, a catalog concept, a stream-resolution concept, or a playback concept?
- am I forcing live and on-demand media into the same fake model?
- am I leaking protocol details into shared page code?

If the streaming spec says a remote feature belongs behind a capability boundary, the UI must not flatten it into ad-hoc controls.

### 3.12 `lofibox-zero-text-input-spec.md`

This document controls:

- what counts as committed text versus input-method composition
- how editable text reaches shared app state
- which Unicode guarantees text-entry pages must preserve
- how X11 and framebuffer/evdev input targets differ
- what belongs to the system input method rather than LoFiBox itself

When writing Search, remote profile field, playlist-name, or future metadata-editing UI code, ask:

- am I mutating app state with committed UTF-8 text or with in-progress preedit text?
- does this behavior belong to the page, SearchState, a platform input adapter, or the user's Debian input-method framework?
- am I accidentally claiming CJK IME support for framebuffer/evdev direct key input?

If the text-input spec says a behavior belongs to the runtime shell or system IME, the page must not implement its own composition engine or ASCII-only shortcut.

### 3.13 `lofibox-zero-page-spec.md`

This document controls:

- what the current UI implementation currently contains
- what the current implementation pages are for
- what elements the current implementation must contain right now
- what actions the current implementation supports right now
- what the current implementation must not contain yet
- what its empty state must do today

This control is split across:

- the shared current UI index at `lofibox-zero-page-spec.md`
- one dedicated page file per page under `docs/specification/current-ui-pages/`

When writing UI code, ask:

- is this element allowed on this page?
- is this interaction required?
- is this page adding unapproved functionality?

If a control or feature is not justified by the current UI index plus the page's dedicated file, it does not belong in the implementation yet.

### 3.14 `lofibox-zero-layout-spec.md`

This document controls:

- the spatial structure of each page on the `320x170` logical screen
- top bar geometry
- content frame geometry
- row density
- zone ownership for cover art, metadata, controls, EQ graph, and information cards

When writing UI code, ask:

- where does each element go?
- how much screen space does it own?
- which page template does this page use?

If the page spec tells you that a page needs a progress bar, the layout spec tells you where it goes.

### 3.15 `lofibox-zero-visual-design-spec.md`

This document controls:

- color tokens
- typography roles
- focus treatment
- chrome treatment
- component appearance
- visual boundaries between content types

When writing UI code, ask:

- how should this element look?
- what color, contrast, and emphasis level should it use?
- how should active, selected, and muted states differ?

If the layout spec tells you where a list row goes, the visual design spec tells you how it should look.

### 3.16 `legacy-lofibox-product-guidance.md`

This document is the fallback product-intent source.

It controls:

- why the product has these pages at all
- what old behaviors are worth carrying forward
- which old mockups are meaningful and which old implementation details are historical accidents

When writing UI code, use it only when the higher-priority `LoFiBox Zero` specifications above are silent.

It must not override them.

## 4. Conflict Resolution

If two documents appear to conflict, resolve them like this:

### 4.1 Final Product Truth Beats Progress Snapshots

- `lofibox-zero-final-product-spec.md` beats current implementation convenience on questions of what the product ultimately is.

Example:

- If the current UI implementation lacks a final product surface such as source management or search, that absence does not redefine the finished product.

### 4.2 Architecture Beats UI Convenience

- `project-architecture-spec.md` beats lower-level implementation specs on questions of code placement and layer ownership.

Example:

- If a visual idea requires physical shell or external harness geometry, it must stay outside shared app code.

### 4.3 Distribution Governance Beats Local Convenience

- Debian/archive, dependency, desktop integration, copyright, plugin/provider, and test governance beat local implementation convenience.

Example:

- If a UI feature needs a new network library, the dependency must be admitted through dependency policy and represented in packaging rather than hidden inside a build script.

### 4.4 Product Shell Beats Generic Desktop Chrome

- LoFiBox's primary product shell is chromeless and widget-like; generic desktop window chrome must not be treated as part of the app UI.

Example:

- The Linux desktop target and the `Cardputer Zero` profile must not display a conventional window menu bar merely because the program is installed as a desktop application.

### 4.5 Persistence Rules Beat Ad-Hoc Save Behavior

- `lofibox-zero-persistence-spec.md` beats convenience on questions of what survives restart, when it loads, and how it is repaired.

Example:

- If a page wants to persist a local interaction detail that the persistence spec marks runtime-only, do not smuggle it into durable storage.

### 4.6 Credential Rules Beat Ad-Hoc Secret Handling

- `lofibox-zero-credential-spec.md` beats convenience on questions of raw secrets, credential references, and session material.

Example:

- If a source profile wants to persist a raw token directly, store a `credential_ref` instead and move the token to the secure secret store.

### 4.7 Indexing Rules Beat Ad-Hoc Scan Behavior

- `lofibox-zero-library-indexing-spec.md` beats convenience on questions of library readiness, recursive traversal, refresh, and rebuild behavior.

Example:

- If Boot wants to pretend the library is ready before indexing truth says so, the indexing spec wins.

### 4.8 Shared App State Beats Page-Local Reinvention

- `lofibox-zero-app-state-spec.md` beats page convenience on questions of playback truth, navigation truth, library context, queue truth, search truth, source truth, network truth, and shared settings truth.

Example:

- If a page wants to own its own copy of playback mode instead of projecting `PlaybackSession`, move that meaning back into shared app state.

### 4.9 Media Pipeline Beats Backend Leakage

- `lofibox-zero-media-pipeline-spec.md` beats page convenience on questions of decode, DSP handoff, and output-path meaning.

Example:

- If a proposed control exposes backend-specific decode details as if they were shared page state, push that detail back behind the pipeline boundary.

### 4.10 Audio DSP Beats Ad-Hoc EQ UI

- `lofibox-zero-audio-dsp-spec.md` beats page convenience on questions of DSP meaning and responsibility.

Example:

- If a proposed EQ feature requires runtime state, preset persistence, and per-device binding, do not fake it as a single page-only slider state blob.

### 4.11 Streaming Capability Beats Ad-Hoc Remote UI

- `lofibox-zero-streaming-spec.md` beats page convenience on questions of remote-media meaning and responsibility.

Example:

- If a proposed control mixes connection management, auth state, and playback state into one convenience widget, split the responsibilities before implementing it.

### 4.12 Text Input Boundary Beats ASCII Convenience

- `lofibox-zero-text-input-spec.md` beats page-local convenience on questions of committed text, preedit, Unicode-safe editing, and system input-method integration.

Example:

- If Search needs Chinese, Japanese, or Korean input on Debian/X11, integrate the X11 target with the session input method and emit committed UTF-8 text. Do not implement a private Pinyin, Kana, or Hangul engine inside the Search page, and do not advertise framebuffer/evdev direct key input as desktop IME support.

### 4.13 Current UI Scope Beats Styling Ideas

- `lofibox-zero-page-spec.md` beats layout and visual ideas on questions of the current UI's content and behavior.

Example:

- If a mockup suggests `Repeat` inside `Settings` but the page spec excludes it, `Repeat` does not go into `Settings`.

### 4.14 Layout Beats Ad-Hoc Composition

- `lofibox-zero-layout-spec.md` beats improvised spacing and positioning decisions.

Example:

- If the layout spec assigns a fixed metadata zone for `Now Playing`, you do not resize or relocate it ad hoc because a single track title is very short.

### 4.15 Visual Spec Beats Local Taste

- `lofibox-zero-visual-design-spec.md` beats one-off styling choices.

Example:

- If the visual spec defines blue focus bars and muted gray metadata, a single page does not get a different highlight color without a spec change.

### 4.16 Legacy Guidance Is Fallback Only

- `legacy-lofibox-product-guidance.md` loses to all newer `LoFiBox Zero` specs when there is direct conflict.

## 5. Implementation Workflow

When implementing a UI page, follow this sequence:

1. Read `lofibox-zero-final-product-spec.md`.
2. Read `debian-official-archive-spec.md` if the work affects build, install, packaging, runtime paths, desktop integration, or dependencies.
3. Read `project-architecture-spec.md` and confirm the code belongs in shared app UI.
4. Read dependency, provider, desktop, copyright, and testing governance specs when the change touches those boundaries.
5. If the work touches save, load, defaults, repair, or persisted server/source settings, read `lofibox-zero-persistence-spec.md`.
6. If the work touches credential references, secret storage, or session rules, read `lofibox-zero-credential-spec.md`.
7. If the work touches library scan, index build, refresh, rebuild, or library readiness, read `lofibox-zero-library-indexing-spec.md`.
8. If the work touches shared playback, navigation, library context, library-index, queue, search, source, credential, network, or settings truth, read `lofibox-zero-app-state-spec.md`.
9. If the work touches decode, playback-pipeline, or output-path behavior, read `lofibox-zero-media-pipeline-spec.md`.
10. If the work touches EQ or DSP behavior, read `lofibox-zero-audio-dsp-spec.md`.
11. If the work touches remote sources, media servers, or network playback, read `lofibox-zero-streaming-spec.md`.
12. If the work touches Search, login fields, source-profile fields, playlist names, or any editable text, read `lofibox-zero-text-input-spec.md`.
13. Read the shared current UI rules in `lofibox-zero-page-spec.md` and then the specific page file under `docs/specification/current-ui-pages/`.
14. Read the page template and geometry rules in `lofibox-zero-layout-spec.md`.
15. Read the component and styling rules in `lofibox-zero-visual-design-spec.md`.
16. Only if still needed, consult `legacy-lofibox-product-guidance.md` for intent clarification.

## 6. Practical Decision Table

| Question | Source of Truth |
| --- | --- |
| What is the finished product actually supposed to be? | `lofibox-zero-final-product-spec.md` |
| What makes this acceptable for Debian and Ubuntu official repositories? | `debian-official-archive-spec.md` |
| Is this dependency acceptable and correctly localized? | `dependency-policy-spec.md` |
| Is this a provider boundary or a core behavior? | `plugin-provider-system-spec.md` |
| Does this desktop behavior belong, and where is it installed? | `linux-desktop-integration-spec.md` |
| Is this a Cardputer Zero profile constraint or product-wide truth? | `cardputer-zero-adaptation-spec.md` |
| Is this asset, fixture, screenshot, or source license-governed? | `copyright-resource-governance-spec.md` |
| Which tests or CI jobs protect this boundary? | `testing-ci-spec.md` |
| What is persisted, when is it loaded, and how is it repaired? | `lofibox-zero-persistence-spec.md` |
| What is a credential reference, what is a secret, and what may persist? | `lofibox-zero-credential-spec.md` |
| What is indexable, when is the library ready, and when do refresh or rebuild apply? | `lofibox-zero-library-indexing-spec.md` |
| Is this truth page-local or shared across the app? | `lofibox-zero-app-state-spec.md` |
| Does this decode, DSP-handoff, or output-path concept belong, and where does it live? | `lofibox-zero-media-pipeline-spec.md` |
| Does this EQ or DSP concept belong, and what responsibility owns it? | `lofibox-zero-audio-dsp-spec.md` |
| What does the current UI implementation contain right now? | `lofibox-zero-page-spec.md` |
| Does this remote-media concept belong, and what responsibility owns it? | `lofibox-zero-streaming-spec.md` |
| Is this committed text, preedit text, raw key input, or system input-method behavior? | `lofibox-zero-text-input-spec.md` |
| Which code layer should own it? | `project-architecture-spec.md` |
| Where does it sit on the screen? | `lofibox-zero-layout-spec.md` |
| What does it look like? | `lofibox-zero-visual-design-spec.md` |
| Why does the product need it? | `legacy-lofibox-product-guidance.md` |

## 7. Change Rule

If implementation needs to deviate from one of these documents:

- do not silently improvise in code
- update the controlling specification first
- then implement the code change

This rule exists specifically to stop UI drift.

## 8. Non-Goals

This hierarchy must not be treated as:

- a replacement for page specifications
- a replacement for visual design tokens
- permission to ignore conflicts because a mockup looks good
- permission to revive old repository structure
