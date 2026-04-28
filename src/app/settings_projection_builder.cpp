// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/settings_projection_builder.h"

namespace lofibox::app {

std::vector<std::pair<std::string, std::string>> buildSettingsProjectionRows(const SettingsProjectionInput& input)
{
    return {
        {"NETWORK", input.network_connected ? "ONLINE" : "OFFLINE"},
        {"METADATA", input.metadata_display_name},
        {"SLEEP TIMER", input.sleep_timer_index == 0 ? "OFF" : "ON"},
        {"BACKLIGHT", std::to_string(input.backlight_index + 1)},
        {"LANGUAGE", "EN"},
        {"REMOTE SETUP", input.remote_sources_available ? std::to_string(input.remote_source_type_count) + " TYPES" : "UNAVAILABLE"},
        {"ABOUT", "INFO"},
    };
}

} // namespace lofibox::app
