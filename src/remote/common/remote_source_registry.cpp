// SPDX-License-Identifier: GPL-3.0-or-later

#include "remote/common/remote_source_registry.h"

namespace lofibox::remote {

bool RemoteSourceRegistry::supported(app::RemoteServerKind kind) const noexcept
{
    switch (kind) {
    case app::RemoteServerKind::Jellyfin:
    case app::RemoteServerKind::OpenSubsonic:
    case app::RemoteServerKind::Navidrome:
    case app::RemoteServerKind::Emby:
        return true;
    }
    return false;
}

bool RemoteSourceRegistry::openSubsonicCompatible(app::RemoteServerKind kind) const noexcept
{
    return kind == app::RemoteServerKind::OpenSubsonic || kind == app::RemoteServerKind::Navidrome;
}

std::string_view RemoteSourceRegistry::providerFamily(app::RemoteServerKind kind) const noexcept
{
    return remoteProviderFamily(kind);
}

RemoteProviderManifest RemoteSourceRegistry::manifest(app::RemoteServerKind kind) const
{
    return remoteProviderManifest(kind);
}

std::vector<RemoteProviderManifest> RemoteSourceRegistry::manifests() const
{
    return {
        manifest(app::RemoteServerKind::Jellyfin),
        manifest(app::RemoteServerKind::OpenSubsonic),
        manifest(app::RemoteServerKind::Navidrome),
        manifest(app::RemoteServerKind::Emby),
    };
}

} // namespace lofibox::remote
