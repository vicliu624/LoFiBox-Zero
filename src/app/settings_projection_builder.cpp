// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/settings_projection_builder.h"

namespace lofibox::app {

std::vector<std::pair<std::string, std::string>> buildSettingsProjectionRows(const SettingsProjectionInput& input)
{
    auto rows = std::vector<std::pair<std::string, std::string>>{
        {"NETWORK", input.network_connected ? "ONLINE" : "OFFLINE"},
        {"METADATA", input.metadata_display_name},
        {"SLEEP TIMER", input.sleep_timer},
        {"BACKLIGHT", std::to_string(input.backlight_index + 1)},
        {"LANGUAGE", "EN"},
    };
    if (input.screen_timeout != "SYSTEM") rows.insert(rows.begin() + 3, {"SCREEN OFF", input.screen_timeout});
    const auto featureValue = [&](RuntimeFeature feature) {
        if (!RuntimePreferences::enabled(input.preference_flags, feature)) return "OFF";
        if (!RuntimePreferences::effective(input.preference_flags, feature)) return "PAUSED";
        return "ON";
    };
    rows.emplace_back("LONG BATTERY", featureValue(RuntimeFeature::BatterySaver));
    rows.emplace_back("SPECTRUM", featureValue(RuntimeFeature::Spectrum));
    if (input.device_lights) {
        rows.emplace_back("LEDS", featureValue(RuntimeFeature::Leds));
        rows.emplace_back("LED ANIMATION", featureValue(RuntimeFeature::LedAnimation));
    }
    rows.emplace_back("CLI / TUI", featureValue(RuntimeFeature::ExternalControl));
    rows.emplace_back("WEBUI", input.webui_url.empty() ? "UNAVAILABLE" : featureValue(RuntimeFeature::WebUi));
    if (!input.webui_url.empty()) {
        if (RuntimePreferences::effective(input.preference_flags, RuntimeFeature::WebUi))
            rows.emplace_back("WEB ADDRESS", input.webui_url);
    }
    rows.emplace_back("SCRAPING", !input.network_connected ? "OFFLINE"
        : RuntimePreferences::enabled(input.preference_flags, RuntimeFeature::BatterySaver) ? "PAUSED" : "AUTO");
    if (input.preferences_error) rows.emplace_back("PREFERENCES", "SAVE FAILED");
    rows.emplace_back("REMOTE SETUP", input.remote_sources_available ? std::to_string(input.remote_source_type_count) + " TYPES" : "UNAVAILABLE");
    rows.emplace_back("ABOUT", "INFO");
    return rows;
}

} // namespace lofibox::app
