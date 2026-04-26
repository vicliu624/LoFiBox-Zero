// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string_view>

#include "app/remote_media_services.h"

namespace lofibox::remote {

class RemoteSourceRegistry {
public:
    [[nodiscard]] bool supported(app::RemoteServerKind kind) const noexcept;
    [[nodiscard]] bool openSubsonicCompatible(app::RemoteServerKind kind) const noexcept;
    [[nodiscard]] std::string_view providerFamily(app::RemoteServerKind kind) const noexcept;
};

} // namespace lofibox::remote
