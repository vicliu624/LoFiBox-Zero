<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero App State Contract: CredentialState

## 1. Purpose

- Own the shared runtime truth for credential-store availability, credential-reference resolution, and runtime auth readiness projections.

## 2. Related Specs

- Shared app-state rules: `docs/specification/lofibox-zero-app-state-spec.md`
- Persistence domains: `docs/specification/lofibox-zero-persistence-spec.md`
- Credential rules: `docs/specification/lofibox-zero-credential-spec.md`
- Source projection: `docs/specification/app-states/source-state.md`

## 3. Owned Truth

- whether the secure credential store is available
- whether persisted credential references can be resolved
- whether runtime session material is currently valid for sources that use it
- whether re-authentication is required for a given source profile

## 4. Data Dependencies

- secure credential store availability
- persisted `credential_ref` records from source profiles
- runtime session/auth outcomes from source integrations

## 5. State Enumeration

- `STATE_CREDENTIALS_UNAVAILABLE`
  secure credential handling is unavailable or unresolved
- `STATE_CREDENTIALS_READY`
  secure credential handling is available and refs may be resolved
- `STATE_CREDENTIALS_DEGRADED`
  credential store or session handling is partially available but some auth projections require repair or re-entry

## 6. Invariants

- Raw secret material must not project into shared UI state.
- Missing or invalid secret material must degrade auth readiness truthfully.
- Source profiles may remain saved even when auth readiness is degraded.
- Session invalidation must not be confused with source deletion.

## 7. Event Contract

- `EVT_CREDENTIAL_STORE_AVAILABLE`
  - effect: mark secure credential handling available
- `EVT_CREDENTIAL_STORE_UNAVAILABLE`
  - effect: mark secure credential handling unavailable
- `EVT_CREDENTIAL_REF_RESOLVED`
  - effect: mark referenced secret resolvable
- `EVT_CREDENTIAL_REF_MISSING`
  - effect: mark referenced secret missing or unusable
- `EVT_SESSION_ESTABLISHED`
  - effect: mark runtime session material valid
- `EVT_SESSION_EXPIRED`
  - effect: mark runtime session material invalid and re-auth required
- `EVT_CREDENTIAL_ROTATED`
  - effect: refresh auth readiness after credential update

## 8. Transition Contract

- `STATE_CREDENTIALS_UNAVAILABLE + EVT_CREDENTIAL_STORE_AVAILABLE` -> effect: enable secure credential handling -> `STATE_CREDENTIALS_READY`
- `STATE_CREDENTIALS_READY + EVT_CREDENTIAL_STORE_UNAVAILABLE` -> effect: degrade secure credential handling -> `STATE_CREDENTIALS_UNAVAILABLE`
- `STATE_CREDENTIALS_READY + EVT_CREDENTIAL_REF_MISSING` -> effect: mark degraded auth readiness -> `STATE_CREDENTIALS_DEGRADED`
- `STATE_CREDENTIALS_READY + EVT_SESSION_EXPIRED` -> effect: mark re-auth required -> `STATE_CREDENTIALS_DEGRADED`
- `STATE_CREDENTIALS_DEGRADED + EVT_CREDENTIAL_REF_RESOLVED` -> effect: restore resolvable secret projection -> `STATE_CREDENTIALS_READY`
- `STATE_CREDENTIALS_DEGRADED + EVT_SESSION_ESTABLISHED` -> effect: restore runtime auth readiness -> `STATE_CREDENTIALS_READY`
- `STATE_CREDENTIALS_DEGRADED + EVT_CREDENTIAL_STORE_UNAVAILABLE` -> effect: mark secure credential handling unavailable -> `STATE_CREDENTIALS_UNAVAILABLE`
- `STATE_CREDENTIALS_READY + EVT_CREDENTIAL_ROTATED` -> effect: refresh auth readiness and session requirements -> `STATE_CREDENTIALS_READY`

## 9. Default State

- Default credential state at app start is `STATE_CREDENTIALS_UNAVAILABLE` until secure credential handling is proven available.

## 10. Error State

- Inaccessible secure storage must not be reported as successful credential readiness.
- Session expiry must not masquerade as valid authenticated state.
- Missing `credential_ref` targets must not silently authenticate a source profile.

## 11. Non-Goals

- Page-level login prompts
- Provider-specific auth UI
- Raw secret presentation in runtime state
