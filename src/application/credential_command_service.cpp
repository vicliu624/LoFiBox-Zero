// SPDX-License-Identifier: GPL-3.0-or-later

#include "application/credential_command_service.h"

#include "app/remote_profile_store.h"

namespace lofibox::application {

CredentialCommandService::CredentialCommandService(const app::RuntimeServices& services) noexcept
    : services_(services)
{
}

CredentialStatus CredentialCommandService::status(const app::RemoteServerProfile& profile) const
{
    CredentialStatus result{};
    result.ref_id = profile.credential_ref.id;
    result.attached = !profile.credential_ref.id.empty();
    result.has_username = !profile.username.empty();
    result.has_password = !profile.password.empty();
    result.has_api_token = !profile.api_token.empty();
    return result;
}

CommandResult CredentialCommandService::setSecret(app::RemoteServerProfile& profile, const CredentialSecretPatch& patch) const
{
    if (profile.id.empty()) {
        return CommandResult::failure("credential.profile-id-missing", "PROFILE ID MISSING");
    }
    if (profile.credential_ref.id.empty()) {
        profile.credential_ref.id = "credential/" + profile.id;
    }
    if (!patch.username.empty()) {
        profile.username = patch.username;
    }
    if (!patch.password.empty()) {
        profile.password = patch.password;
    }
    if (!patch.api_token.empty()) {
        profile.api_token = patch.api_token;
    }
    if (!services_.remote.remote_profile_store->saveCredentials(profile)) {
        return CommandResult::failure("credential.save-failed", "SAVE FAIL");
    }
    return CommandResult::success("credential.saved", "SAVED");
}

CommandResult CredentialCommandService::deleteSecret(app::RemoteServerProfile& profile) const
{
    if (profile.credential_ref.id.empty()) {
        profile.password.clear();
        profile.api_token.clear();
        return CommandResult::success("credential.none", "NONE");
    }
    if (!services_.remote.remote_profile_store->deleteCredentials(profile.credential_ref)) {
        return CommandResult::failure("credential.delete-failed", "DELETE FAIL");
    }
    profile.password.clear();
    profile.api_token.clear();
    return CommandResult::success("credential.deleted", "DELETED");
}

} // namespace lofibox::application
