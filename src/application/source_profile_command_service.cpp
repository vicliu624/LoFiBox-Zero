// SPDX-License-Identifier: GPL-3.0-or-later

#include "application/source_profile_command_service.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>

#include "app/remote_profile_store.h"
#include "remote/common/remote_provider_contract.h"
#include "remote/common/remote_source_registry.h"

namespace lofibox::application {

SourceProfileCommandService::SourceProfileCommandService(const app::RuntimeServices& services) noexcept
    : services_(services)
{
}

std::vector<app::RemoteServerProfile> SourceProfileCommandService::loadProfiles() const
{
    return services_.remote.remote_profile_store->loadProfiles();
}

bool SourceProfileCommandService::persistProfiles(std::vector<app::RemoteServerProfile>& profiles) const
{
    for (auto& profile : profiles) {
        ensureCredentialRef(profile, profiles.size());
    }
    return services_.remote.remote_profile_store->saveProfiles(profiles);
}

bool SourceProfileCommandService::saveCredentials(app::RemoteServerProfile& profile, std::size_t profile_count) const
{
    ensureCredentialRef(profile, profile_count);
    if (!supportsCredentials(profile.kind)) {
        return true;
    }
    return services_.remote.remote_profile_store->saveCredentials(profile);
}

app::RemoteServerProfile SourceProfileCommandService::createProfile(app::RemoteServerKind kind, std::size_t existing_profile_count) const
{
    app::RemoteServerProfile profile{};
    profile.kind = kind;
    profile.id = defaultProfileId(kind, existing_profile_count);
    profile.name = defaultProfileName(kind);
    profile.tls_policy.verify_peer = true;
    ensureCredentialRef(profile, existing_profile_count + 1U);
    return profile;
}

std::size_t SourceProfileCommandService::ensureProfileForKind(std::vector<app::RemoteServerProfile>& profiles, app::RemoteServerKind kind) const
{
    if (const auto existing = findProfileByKind(profiles, kind)) {
        return *existing;
    }
    profiles.push_back(createProfile(kind, profiles.size()));
    return profiles.size() - 1U;
}

std::optional<std::size_t> SourceProfileCommandService::findProfileByKind(const std::vector<app::RemoteServerProfile>& profiles, app::RemoteServerKind kind) const
{
    const auto it = std::find_if(profiles.begin(), profiles.end(), [kind](const app::RemoteServerProfile& profile) {
        return profile.kind == kind;
    });
    if (it == profiles.end()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(std::distance(profiles.begin(), it));
}

void SourceProfileCommandService::ensureCredentialRef(app::RemoteServerProfile& profile, std::size_t profile_count) const
{
    if (profile.id.empty()) {
        profile.id = defaultProfileId(profile.kind, profile_count);
    }
    if (!supportsCredentials(profile.kind)) {
        profile.credential_ref.id.clear();
        return;
    }
    if (profile.credential_ref.id.empty()) {
        profile.credential_ref.id = "credential/" + profile.id;
    }
}

bool SourceProfileCommandService::updateTextField(app::RemoteServerProfile& profile, SourceProfileTextField field, std::string_view value, std::size_t profile_count) const
{
    switch (field) {
    case SourceProfileTextField::Label:
        profile.name = std::string{value};
        return true;
    case SourceProfileTextField::Address:
        profile.base_url = std::string{value};
        return true;
    case SourceProfileTextField::Username:
        profile.username = std::string{value};
        return true;
    case SourceProfileTextField::Password:
        ensureCredentialRef(profile, profile_count);
        profile.password = std::string{value};
        return saveCredentials(profile, profile_count);
    case SourceProfileTextField::ApiToken:
        ensureCredentialRef(profile, profile_count);
        profile.api_token = std::string{value};
        return saveCredentials(profile, profile_count);
    }
    return false;
}

bool SourceProfileCommandService::toggleTlsVerify(app::RemoteServerProfile& profile, std::vector<app::RemoteServerProfile>& profiles) const
{
    profile.tls_policy.verify_peer = !profile.tls_policy.verify_peer;
    if (profile.tls_policy.verify_peer) {
        profile.tls_policy.allow_self_signed = false;
    }
    return persistProfiles(profiles);
}

bool SourceProfileCommandService::toggleSelfSigned(app::RemoteServerProfile& profile, std::vector<app::RemoteServerProfile>& profiles) const
{
    profile.tls_policy.allow_self_signed = !profile.tls_policy.allow_self_signed;
    if (profile.tls_policy.allow_self_signed) {
        profile.tls_policy.verify_peer = false;
    }
    return persistProfiles(profiles);
}

SourceProfileProbeResult SourceProfileCommandService::probe(app::RemoteServerProfile& profile, std::size_t profile_count) const
{
    ensureCredentialRef(profile, profile_count);
    auto session = services_.remote.remote_source_provider->probe(profile);
    return {
        session.available
            ? CommandResult::success("source-profile.probe.online", "ONLINE")
            : CommandResult::failure("source-profile.probe.failed", session.message.empty() ? "FAILED" : session.message),
        std::move(session)};
}

std::string SourceProfileCommandService::kindDisplayName(app::RemoteServerKind kind) const
{
    const auto manifest = remote::remoteProviderManifest(kind);
    return manifest.display_name.empty() ? app::remoteServerKindToString(kind) : manifest.display_name;
}

std::string SourceProfileCommandService::defaultProfileId(app::RemoteServerKind kind, std::size_t index) const
{
    return app::remoteServerKindToString(kind) + "-" + std::to_string(static_cast<int>(index + 1U));
}

std::string SourceProfileCommandService::defaultProfileName(app::RemoteServerKind kind) const
{
    auto name = kindDisplayName(kind);
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return name;
}

std::string SourceProfileCommandService::profileLabel(const app::RemoteServerProfile& profile) const
{
    if (!profile.name.empty()) {
        return profile.name;
    }
    if (!profile.base_url.empty()) {
        return profile.base_url;
    }
    return defaultProfileName(profile.kind);
}

SourceProfileCredentialMode SourceProfileCommandService::credentialMode(app::RemoteServerKind kind) const noexcept
{
    switch (kind) {
    case app::RemoteServerKind::Jellyfin:
    case app::RemoteServerKind::OpenSubsonic:
    case app::RemoteServerKind::Navidrome:
    case app::RemoteServerKind::Emby:
        return SourceProfileCredentialMode::Required;
    case app::RemoteServerKind::PlaylistManifest:
    case app::RemoteServerKind::WebDav:
    case app::RemoteServerKind::Ftp:
    case app::RemoteServerKind::Sftp:
        return SourceProfileCredentialMode::Optional;
    case app::RemoteServerKind::DirectUrl:
    case app::RemoteServerKind::InternetRadio:
    case app::RemoteServerKind::Hls:
    case app::RemoteServerKind::Dash:
    case app::RemoteServerKind::Smb:
    case app::RemoteServerKind::Nfs:
    case app::RemoteServerKind::DlnaUpnp:
        return SourceProfileCredentialMode::None;
    }
    return SourceProfileCredentialMode::None;
}

bool SourceProfileCommandService::supportsCredentials(app::RemoteServerKind kind) const noexcept
{
    return credentialMode(kind) != SourceProfileCredentialMode::None;
}

std::string SourceProfileCommandService::readiness(const app::RemoteServerProfile& profile) const
{
    if (profile.base_url.empty()) {
        return "NEEDS URL";
    }
    if (credentialMode(profile.kind) != SourceProfileCredentialMode::Required) {
        return "READY";
    }
    if (profile.username.empty()) {
        return "NEEDS USER";
    }
    if (profile.password.empty()
        && profile.api_token.empty()
        && profile.credential_ref.id.empty()) {
        return "NEEDS SECRET";
    }
    return "READY";
}

std::string SourceProfileCommandService::credentialValueLabel(app::RemoteServerKind kind, std::string_view value) const
{
    const auto mode = credentialMode(kind);
    if (mode == SourceProfileCredentialMode::None) {
        return "N/A";
    }
    if (value.empty() && mode == SourceProfileCredentialMode::Optional) {
        return "OPTIONAL";
    }
    return value.empty() ? "EMPTY" : "SET";
}

std::string SourceProfileCommandService::usernameLabel(const app::RemoteServerProfile& profile) const
{
    const auto mode = credentialMode(profile.kind);
    if (mode == SourceProfileCredentialMode::None) {
        return "N/A";
    }
    if (profile.username.empty()) {
        return mode == SourceProfileCredentialMode::Optional ? "OPTIONAL" : "MISSING";
    }
    return profile.username;
}

std::string SourceProfileCommandService::permissionLabel(app::RemoteServerKind kind) const
{
    const auto manifest = remote::remoteProviderManifest(kind);
    if (remote::remoteProviderHasCapability(manifest, remote::RemoteProviderCapability::WritableMetadata)
        || remote::remoteProviderHasCapability(manifest, remote::RemoteProviderCapability::WritableFavorites)) {
        return "READ/WRITE";
    }
    return "READ ONLY";
}

bool SourceProfileCommandService::keepsLocalFacts(app::RemoteServerKind kind) const
{
    const auto manifest = remote::remoteProviderManifest(kind);
    const bool writable_metadata =
        remote::remoteProviderHasCapability(manifest, remote::RemoteProviderCapability::WritableMetadata)
        && !remote::remoteProviderHasCapability(manifest, remote::RemoteProviderCapability::ReadOnly);
    return !writable_metadata;
}

} // namespace lofibox::application
