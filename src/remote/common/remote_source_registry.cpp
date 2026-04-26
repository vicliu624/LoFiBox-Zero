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
    switch (kind) {
    case app::RemoteServerKind::Jellyfin: return "jellyfin";
    case app::RemoteServerKind::OpenSubsonic: return "opensubsonic";
    case app::RemoteServerKind::Navidrome: return "opensubsonic";
    case app::RemoteServerKind::Emby: return "emby";
    }
    return "unsupported";
}

} // namespace lofibox::remote
