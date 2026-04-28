<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Runtime Command And Session Architecture

## 1. Purpose

This document defines the live runtime command/query boundary for `LoFiBox Zero`.

The boundary exists because live playback, active queue, active EQ, active remote stream state, and immediately effective runtime settings are not durable configuration and are not GUI page state.
They belong to the running LoFiBox instance.

Runtime CLI is only one future entry point into this boundary.
The same boundary must serve GUI input, desktop integration, future runtime CLI, future automation clients, MCP-style runtime tools, and tests.

## 2. Authority

For enduring source ownership, use `project-architecture-spec.md`.
For durable product commands, use `application-command-boundary-spec.md`.
For shared app-state contracts, use `lofibox-zero-app-state-spec.md`.
For playback behavior, use `lofibox-zero-media-pipeline-spec.md` and `lofibox-zero-final-product-spec.md`.
For EQ/DSP behavior, use `lofibox-zero-audio-dsp-spec.md`.
For streaming behavior, use `lofibox-zero-streaming-spec.md`.
For testing gates, use `testing-ci-spec.md`.

This document defines the runtime command/session layer that sits between interface adapters and the running live domains.

## 3. Distinction Contract

### 3.1 Current Confusions

- `runtime CLI` is being confused with a CLI module. It is an external entry into the runtime command bus.
- `AppRuntimeContext` is being confused with the owner of live playback, queue, EQ, and remote session truth.
- GUI page commands such as selected-row confirm, EQ selected-band movement, and page-local stream detail confirmation are being confused with live runtime commands.
- UI rows such as queue rows, stream detail rows, and diagnostics rows are being confused with structured runtime snapshots.
- IPC transport is being confused with the runtime command protocol.
- direct CLI commands are being confused with commands that mutate the running runtime instance.

### 3.2 Valid Distinctions

- `InterfaceAdapter` receives input from GUI, CLI, desktop, MCP, automation, or tests and submits runtime commands or queries.
- `RuntimeCommandBus` is the transport-neutral in-process boundary that accepts live runtime commands and runtime queries.
- `RuntimeCommandDispatcher` serializes and applies live runtime commands to the running instance.
- `RuntimeQueryDispatcher` returns structured live runtime snapshots.
- `RuntimeCommand` is a mutation request against live runtime truth.
- `RuntimeQuery` is a read request for live runtime truth.
- `RuntimeCommandResult` is the structured outcome of a runtime command.
- `RuntimeSnapshot` is structured live truth before GUI rows, terminal text, desktop notifications, or automation payloads.
- `RuntimeSessionFacade` is the process-local facade over playback runtime, queue runtime, EQ runtime, remote session runtime, and live settings runtime.
- `RuntimeCommandClient` and `RuntimeCommandServer` are future transport-facing wrappers around the same runtime command/query contract.

### 3.3 Invalid Distinctions

- Do not make runtime CLI call `AppRuntimeContext`, page confirm methods, or page field editors.
- Do not make GUI-only selected row or selected EQ band part of runtime truth.
- Do not make UI row text the command/query schema for runtime CLI, desktop integration, or automation.
- Do not let a second process mutate playback, active queue, active EQ, or current remote stream by writing durable state.
- Do not introduce IPC before the transport-neutral command/query/result/snapshot contract exists.
- Do not let transport details such as Unix sockets, D-Bus, JSON-RPC, stdio, or named pipes define product command semantics.

## 4. Normative Architecture

The runtime command architecture is:

```text
GUI / runtime CLI / desktop / MCP / automation / tests
        |
        v
Runtime Command And Query Bus
        |
        v
Runtime Session Facade
        |
        v
Playback Runtime / Queue Runtime / EQ Runtime / Remote Session Runtime / Settings Runtime
        |
        v
Application Services / Controllers / RuntimeServices
```

The direct CLI remains outside this live runtime layer:

```text
argv -> DirectCliDispatcher -> AppServiceHost/AppServiceRegistry -> durable application services
```

The future runtime CLI path must become:

```text
argv -> RuntimeCommandClient -> RuntimeCommandServer -> RuntimeCommandBus -> RuntimeSessionFacade
```

## 5. Runtime Domains

### 5.1 Playback Runtime

Playback runtime owns:

- current playback status
- current local track id or current stream identity
- current source label
- elapsed and duration facts
- audio-active and failure/recovery facts that are live-session facts
- playback transport mutations: play, pause, resume, toggle, stop, seek, next, previous

Playback runtime does not own:

- page selection
- Now Playing page UI state
- library browse context
- durable library indexing

### 5.2 Queue Runtime

Queue runtime owns:

- active queue ids
- active queue index
- queue mode facts such as shuffle, repeat-all, and repeat-one when they affect the active queue
- queue mutation version

Queue runtime does not own:

- songs page context
- library list projection
- playlist editor page selection

### 5.3 EQ Runtime

EQ runtime owns:

- currently effective DSP/EQ profile
- enabled or disabled state
- current 10-band gains
- active preset name or custom marker
- runtime-applied DSP update version

EQ runtime does not own:

- Equalizer page selected band
- Equalizer page cursor/focus/scroll state
- small-screen layout or help text

### 5.4 Remote Session Runtime

Remote session runtime owns:

- current active remote profile identity when a remote stream is selected or playing
- current remote source session facts
- current resolved stream diagnostics
- buffer, degraded, recovery, quality, codec, bitrate, live, and seekable facts for the active stream

Remote session runtime does not own:

- durable source profile fields
- credential secret lifecycle
- remote browse page selection
- remote catalog cache persistence policy

Source configuration remains owned by `SourceProfileCommandService`.
Remote browse/search/resolve query behavior remains owned by `RemoteBrowseQueryService`.
Remote session runtime owns only the active live-session truth.

### 5.5 Live Settings Runtime

Live settings runtime owns only settings that immediately affect a running session, such as active output mode, active network policy, active sleep timer, or active projection mode once those settings exist.

Durable settings persistence remains an application/persistence concern.

## 6. Command And Query Contract

Runtime commands must be represented by an explicit command object with:

- `RuntimeCommandKind`
- `RuntimeCommandPayload`
- `CommandOrigin`
- `CorrelationId`

Runtime queries must be represented by an explicit query object with:

- `RuntimeQueryKind`
- `CommandOrigin`
- `CorrelationId`

Runtime command results must include:

- `accepted`
- `applied`
- stable `code`
- human `message`
- `correlation_id`
- `version_after_apply`

Runtime snapshots must be structured objects, not pre-rendered text.
GUI rows, terminal output, desktop notifications, and automation payloads are projections over runtime snapshots.

## 7. Serialization And Instance Boundary

All live runtime mutations must pass through one serialized runtime command dispatcher in the running instance.

This applies equally to:

- GUI input
- desktop media keys
- future runtime CLI
- future automation
- future MCP runtime tools

The single-instance lock prevents multiple live runtime owners.
It does not authorize another process to mutate live runtime state directly.

## 8. Source Placement

Runtime command/session code belongs under `src/runtime/`.

Required first-stage files are:

```text
src/runtime/
  runtime_command.*
  runtime_result.*
  runtime_snapshot.*
  runtime_session_facade.*
  runtime_command_dispatcher.*
  runtime_query_dispatcher.*
  runtime_command_bus.*
```

Future transport-facing wrappers belong under a platform transport boundary, not in `src/cli` and not in `src/app`.

## 9. Relationship To GUI

`AppRuntimeContext` remains the GUI runtime shell.

It may own:

- GUI input routing
- page navigation
- page-local selection/focus/help state
- page projection assembly
- temporary GUI browse state while a dedicated runtime domain is not yet owning it

It must not be the stable runtime command API.

Whenever GUI input changes live playback, active queue, active EQ, active remote session, or live settings, the page interpreter must submit a runtime command.
GUI-only motion such as moving a list selection or moving the selected EQ band may remain page state.

## 10. First Implementation Scope

The first implementation step should establish the in-process runtime command/query bus and route GUI live playback, queue stepping, and active EQ mutation through it.

It should not implement external runtime CLI or IPC.

It should expose structured snapshots for:

- playback
- queue
- EQ
- active remote session

It should update tests and machine gates so future code cannot route runtime CLI or GUI live mutations around this boundary.

## 11. AI Constraints

- Do not implement runtime CLI commands until this runtime bus exists and tests prove GUI can use it.
- Do not add transport before the transport-neutral command/query/result/snapshot contract is in place.
- Do not let `src/cli` include runtime internals for live-state control; future runtime CLI must use a runtime command client.
- Do not add live playback, queue, EQ, or remote-session mutation methods to `AppRuntimeContext`.
- Do not allow UI rows to become runtime command/query payloads.
