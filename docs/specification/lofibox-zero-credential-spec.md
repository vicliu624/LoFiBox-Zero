<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# LoFiBox Zero Credential And Secret Handling Specification

## 1. Purpose

This document defines how `LoFiBox Zero` stores, resolves, rotates, invalidates, and clears credentials and other secret material.

It exists to turn `credential_ref` into a real contract instead of a placeholder word.
The product must explicitly distinguish:

- persisted credential references
- secret material stored in a secure store
- runtime session material
- user-visible auth failures and re-entry requirements

## 2. Authority

For final product meaning, use `lofibox-zero-final-product-spec.md`.
For persistence-domain boundaries, use `lofibox-zero-persistence-spec.md`.
For shared runtime truth, use `lofibox-zero-app-state-spec.md`.

This document defines secret-handling and credential-lifecycle rules that persistence and runtime state must obey.

## 3. Scope

This document covers:

- credential classes
- credential reference model
- secure-store requirements
- runtime session material rules
- credential rotation and invalidation rules
- credential clearing and deletion rules
- redaction and export rules

This document does not define:

- cryptographic algorithm implementation
- OS-specific keychain APIs
- page-level auth prompts
- server-specific auth protocol internals

## 4. Credential Classes

`LoFiBox Zero` should recognize these secret classes:

- username and password
- token
- API key
- session or cookie material
- other secret bearer material required by a supported source family

These classes may map to different server or source families, but they must not be flattened into generic page text fields without an owning credential model.

## 5. Credential Reference Model

### 5.1 `credential_ref`

Persisted product records must store a `credential_ref` rather than raw secret material.

A `credential_ref` should be able to identify:

- credential record id
- credential class
- owning source or server profile id
- optional secret-store namespace or scope discriminator

### 5.2 What May Persist In Ordinary Product Records

Ordinary product records may persist:

- `credential_ref`
- `auth_mode`
- server or source identity
- user-configured TLS policy
- non-secret capability preferences

Ordinary product records must not persist:

- raw passwords
- raw tokens
- raw API keys
- raw cookie blobs
- raw session blobs

## 6. Secure Store Rules

### 6.1 Secret Store Requirement

Secret material must live in a dedicated secure credential store or equivalent protected container outside ordinary product configuration records.

### 6.2 Secret Store Responsibilities

The secure store should support:

- create secret by `credential_ref`
- update or rotate secret by `credential_ref`
- resolve secret by `credential_ref`
- delete secret by `credential_ref`
- report missing, invalid, or inaccessible secret state

### 6.3 Redaction Rule

Secret material must always be redacted in:

- UI detail surfaces
- logs
- debug readouts
- exported ordinary configuration payloads

## 7. Runtime Session Material

### 7.1 Session Versus Credential Distinction

Long-lived credentials and runtime sessions are different.

- durable credentials may persist via `credential_ref`
- ephemeral session cookies or temporary session tokens are runtime material by default

### 7.2 Session Persistence Rule

Ephemeral session material must remain runtime-only unless a later security policy explicitly permits durable session restoration for a specific source family.

### 7.3 Session Loss Rule

If runtime session material expires or becomes invalid:

- the source profile remains valid as a saved source
- runtime availability may degrade
- the system may request re-authentication through the owning source flow

## 8. Credential Lifecycle Rules

### 8.1 Create

Credential creation should:

1. create or update a source/server profile
2. create secure secret material in the credential store
3. persist `credential_ref` in the product record
4. project runtime auth readiness from the combined durable and secure state

### 8.2 Rotate Or Update

Credential rotation should:

1. update secure secret material for the same `credential_ref` or replace it deterministically
2. invalidate dependent runtime sessions when required
3. preserve or repair source-profile linkage

### 8.3 Delete

Deleting a credential should:

1. remove secure secret material
2. clear or invalidate the corresponding `credential_ref`
3. degrade source auth readiness truthfully

### 8.4 Source Deletion Rule

Deleting a saved source or server profile should also clear its credential reference and any associated secure secret material, unless a later explicit policy says otherwise.

## 9. Repair And Fallback Rules

### 9.1 Dangling Reference

If a persisted `credential_ref` points to missing secret material:

- source identity may still load
- auth readiness must degrade truthfully
- the product must not pretend the source is authenticated

### 9.2 Inaccessible Secret Store

If the secure store is unavailable:

- persisted source profiles may still load as records
- credential-backed auth readiness must degrade
- runtime sessions depending on secret resolution must not be fabricated

### 9.3 Invalid Secret Format

Invalid or corrupted secret material must be treated as unusable and should trigger re-entry or repair rather than silent partial use.

## 10. Export And Import Rules

Ordinary exports may include:

- source profile identities
- `auth_mode`
- `credential_ref` only when it is intentionally portable and not security-sensitive

Ordinary exports must not include:

- raw secret values
- raw session material

If imports encounter unresolved or non-portable `credential_ref` values, the imported sources must degrade to unauthenticated saved profiles until credentials are re-entered.

## 11. Relationship To Other Specs

- `lofibox-zero-persistence-spec.md` defines what persistence domains exist and when they load.
- this document defines how credential references and secret material behave within those domains.
- `SourceState` may project auth readiness, but it must not own raw secret material.
- `PersistenceState` may report degraded persistence, but it must not expose raw secret values.

## 12. Non-Goals

- Choosing a specific operating-system keychain
- UI copy for auth prompts
- Implementing provider-specific OAuth or similar flows in this document
