// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <utility>
#include <vector>

#include "app/remote_media_services.h"
#include "remote/common/remote_provider_contract.h"

namespace lofibox::app {

[[nodiscard]] std::vector<std::pair<std::string, std::string>> buildSourceManagerRows(const std::vector<RemoteServerProfile>& profiles);
[[nodiscard]] std::vector<std::pair<std::string, std::string>> buildSourceManagerRows(
    const std::vector<RemoteServerProfile>& profiles,
    const std::vector<remote::RemoteProviderManifest>& manifests);

} // namespace lofibox::app
