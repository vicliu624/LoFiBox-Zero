// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/remote_profile_store.h"

#include <algorithm>

namespace lofibox::app {

std::string remoteServerKindToString(RemoteServerKind kind)
{
    switch (kind) {
    case RemoteServerKind::Jellyfin: return "jellyfin";
    case RemoteServerKind::OpenSubsonic: return "opensubsonic";
    case RemoteServerKind::Navidrome: return "navidrome";
    case RemoteServerKind::Emby: return "emby";
    }
    return "jellyfin";
}

RemoteServerKind remoteServerKindFromString(const std::string& value)
{
    std::string lower = value;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    if (lower == "opensubsonic" || lower == "subsonic") {
        return RemoteServerKind::OpenSubsonic;
    }
    if (lower == "navidrome") {
        return RemoteServerKind::Navidrome;
    }
    if (lower == "emby") {
        return RemoteServerKind::Emby;
    }
    return RemoteServerKind::Jellyfin;
}

RemoteServerProfile sanitizeRemoteProfileForPersistence(RemoteServerProfile profile)
{
    profile.password.clear();
    profile.api_token.clear();
    return profile;
}

} // namespace lofibox::app
