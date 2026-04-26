// SPDX-License-Identifier: GPL-3.0-or-later

#include "remote/common/remote_provider_contract.h"

#include <algorithm>

namespace lofibox::remote {

std::string_view remoteServerKindId(app::RemoteServerKind kind) noexcept
{
    switch (kind) {
    case app::RemoteServerKind::Jellyfin: return "jellyfin";
    case app::RemoteServerKind::OpenSubsonic: return "opensubsonic";
    case app::RemoteServerKind::Navidrome: return "navidrome";
    case app::RemoteServerKind::Emby: return "emby";
    }
    return "unsupported";
}

std::string_view remoteProviderFamily(app::RemoteServerKind kind) noexcept
{
    switch (kind) {
    case app::RemoteServerKind::Jellyfin: return "jellyfin";
    case app::RemoteServerKind::OpenSubsonic:
    case app::RemoteServerKind::Navidrome:
        return "opensubsonic";
    case app::RemoteServerKind::Emby: return "emby";
    }
    return "unsupported";
}

bool remoteProviderHasCapability(const RemoteProviderManifest& manifest, RemoteProviderCapability capability) noexcept
{
    return std::find(manifest.capabilities.begin(), manifest.capabilities.end(), capability) != manifest.capabilities.end();
}

RemoteProviderManifest remoteProviderManifest(app::RemoteServerKind kind)
{
    RemoteProviderManifest manifest{};
    manifest.kind = kind;
    manifest.type_id = std::string(remoteServerKindId(kind));
    manifest.family = std::string(remoteProviderFamily(kind));
    manifest.capabilities = {
        RemoteProviderCapability::Probe,
        RemoteProviderCapability::SearchTracks,
        RemoteProviderCapability::RecentTracks,
        RemoteProviderCapability::ResolveStream,
        RemoteProviderCapability::ReadOnly,
    };

    switch (kind) {
    case app::RemoteServerKind::Jellyfin:
        manifest.display_name = "Jellyfin";
        manifest.capabilities.push_back(RemoteProviderCapability::BrowseCatalog);
        break;
    case app::RemoteServerKind::OpenSubsonic:
        manifest.display_name = "OpenSubsonic";
        manifest.capabilities.push_back(RemoteProviderCapability::BrowseCatalog);
        break;
    case app::RemoteServerKind::Navidrome:
        manifest.display_name = "Navidrome";
        manifest.capabilities.push_back(RemoteProviderCapability::BrowseCatalog);
        break;
    case app::RemoteServerKind::Emby:
        manifest.display_name = "Emby";
        manifest.capabilities.push_back(RemoteProviderCapability::BrowseCatalog);
        break;
    }
    return manifest;
}

} // namespace lofibox::remote
