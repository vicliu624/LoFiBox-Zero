// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string_view>
#include <vector>

#include "app/remote_media_services.h"
#include "remote/common/remote_provider_contract.h"

namespace lofibox::remote {

class RemoteSourceRegistry {
public:
    [[nodiscard]] bool supported(app::RemoteServerKind kind) const noexcept;
    [[nodiscard]] bool openSubsonicCompatible(app::RemoteServerKind kind) const noexcept;
    [[nodiscard]] std::string_view providerFamily(app::RemoteServerKind kind) const noexcept;
    [[nodiscard]] RemoteProviderManifest manifest(app::RemoteServerKind kind) const;
    [[nodiscard]] std::vector<RemoteProviderManifest> manifests() const;
};

} // namespace lofibox::remote
