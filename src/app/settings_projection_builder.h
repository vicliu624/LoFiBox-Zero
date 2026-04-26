// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <utility>
#include <vector>

namespace lofibox::app {

struct SettingsProjectionInput {
    bool network_connected{false};
    std::string metadata_display_name{};
    int sleep_timer_index{0};
    int backlight_index{0};
    bool remote_sources_available{false};
};

[[nodiscard]] std::vector<std::pair<std::string, std::string>> buildSettingsProjectionRows(const SettingsProjectionInput& input);

} // namespace lofibox::app
