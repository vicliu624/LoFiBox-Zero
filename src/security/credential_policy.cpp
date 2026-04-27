// SPDX-License-Identifier: GPL-3.0-or-later

#include "security/credential_policy.h"

#include <algorithm>
#include <array>
#include <utility>

namespace lofibox::security {

std::string SecretRedactor::redact(std::string value) const
{
    static constexpr std::array<std::string_view, 5> markers{
        "password=",
        "api_key=",
        "token=",
        "access_token=",
        "Authorization:"};
    for (const auto marker : markers) {
        std::size_t pos = 0;
        while ((pos = value.find(marker, pos)) != std::string::npos) {
            const auto begin = pos + marker.size();
            auto end = value.find_first_of("& \n\r\t", begin);
            if (end == std::string::npos) {
                end = value.size();
            }
            value.replace(begin, end - begin, "<redacted>");
            pos = begin + 10;
        }
    }
    return value;
}

bool CredentialStore::save(CredentialRecord record)
{
    if (record.ref.id.empty()) {
        return false;
    }
    auto found = std::find_if(records_.begin(), records_.end(), [&](const auto& existing) {
        return existing.ref.id == record.ref.id;
    });
    if (found == records_.end()) {
        records_.push_back(std::move(record));
    } else {
        *found = std::move(record);
    }
    return true;
}

std::optional<CredentialRecord> CredentialStore::load(const CredentialRef& ref) const
{
    if (std::find(revoked_ids_.begin(), revoked_ids_.end(), ref.id) != revoked_ids_.end()) {
        return std::nullopt;
    }
    const auto found = std::find_if(records_.begin(), records_.end(), [&](const auto& record) {
        return record.ref.id == ref.id;
    });
    if (found == records_.end()) {
        return std::nullopt;
    }
    return *found;
}

bool CredentialStore::revoke(const CredentialRef& ref)
{
    if (ref.id.empty()) {
        return false;
    }
    revoked_ids_.push_back(ref.id);
    return true;
}

CredentialStatus CredentialStore::status(const CredentialRef& ref) const
{
    if (std::find(revoked_ids_.begin(), revoked_ids_.end(), ref.id) != revoked_ids_.end()) {
        return {CredentialLifecycleState::Revoked, false, "Credential was revoked."};
    }
    const auto record = load(ref);
    if (!record) {
        return {CredentialLifecycleState::Missing, false, "Credential is missing."};
    }
    const bool writable = std::find(record->permissions.begin(), record->permissions.end(), CredentialPermission::WriteFavorite) != record->permissions.end()
        || std::find(record->permissions.begin(), record->permissions.end(), CredentialPermission::WritePlaybackHistory) != record->permissions.end();
    return {CredentialLifecycleState::Valid, writable, "Credential is valid."};
}

std::string credentialPermissionLabel(CredentialPermission permission)
{
    switch (permission) {
    case CredentialPermission::ReadCatalog: return "READ CATALOG";
    case CredentialPermission::ResolveStream: return "RESOLVE STREAM";
    case CredentialPermission::WriteFavorite: return "WRITE FAVORITES";
    case CredentialPermission::WritePlaybackHistory: return "WRITE HISTORY";
    }
    return "UNKNOWN";
}

} // namespace lofibox::security
