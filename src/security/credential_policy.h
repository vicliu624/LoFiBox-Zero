// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>
#include <string>
#include <vector>

namespace lofibox::security {

struct CredentialRef {
    std::string id{};
};

struct TlsPolicy {
    bool verify_peer{true};
    bool allow_self_signed{false};
};

class SecretRedactor {
public:
    [[nodiscard]] std::string redact(std::string value) const;
};

enum class CredentialPermission {
    ReadCatalog,
    ResolveStream,
    WriteFavorite,
    WritePlaybackHistory,
};

struct CredentialRecord {
    CredentialRef ref{};
    std::string service{};
    std::string username{};
    std::string token{};
    std::vector<CredentialPermission> permissions{};
};

enum class CredentialLifecycleState {
    Missing,
    Valid,
    Expired,
    Revoked,
};

struct CredentialStatus {
    CredentialLifecycleState state{CredentialLifecycleState::Missing};
    bool writable{false};
    std::string message{};
};

class CredentialStore {
public:
    bool save(CredentialRecord record);
    [[nodiscard]] std::optional<CredentialRecord> load(const CredentialRef& ref) const;
    bool revoke(const CredentialRef& ref);
    [[nodiscard]] CredentialStatus status(const CredentialRef& ref) const;

private:
    std::vector<CredentialRecord> records_{};
    std::vector<std::string> revoked_ids_{};
};

[[nodiscard]] std::string credentialPermissionLabel(CredentialPermission permission);

} // namespace lofibox::security
