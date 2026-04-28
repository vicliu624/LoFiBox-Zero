// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

#include "app/remote_media_services.h"

namespace lofibox::app {

class RemoteProfileStore {
public:
    virtual ~RemoteProfileStore() = default;

    [[nodiscard]] virtual std::vector<RemoteServerProfile> loadProfiles() const = 0;
    virtual bool saveProfiles(const std::vector<RemoteServerProfile>& profiles) const = 0;
    virtual bool saveCredentials(const RemoteServerProfile& profile) const
    {
        (void)profile;
        return false;
    }
};

[[nodiscard]] std::string remoteServerKindToString(RemoteServerKind kind);
[[nodiscard]] RemoteServerKind remoteServerKindFromString(const std::string& value);
[[nodiscard]] RemoteServerProfile sanitizeRemoteProfileForPersistence(RemoteServerProfile profile);

} // namespace lofibox::app
