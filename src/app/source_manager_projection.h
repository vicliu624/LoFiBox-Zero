// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <utility>
#include <vector>

#include "app/remote_media_services.h"

namespace lofibox::app {

[[nodiscard]] std::vector<std::pair<std::string, std::string>> buildSourceManagerRows(const std::vector<RemoteServerProfile>& profiles);

} // namespace lofibox::app
