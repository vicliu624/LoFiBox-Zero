// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <string>

namespace lofibox::app {

struct SettingsState {
    int sleep_timer_index{0};
    int backlight_index{1};
    int language_index{0};
};

struct NetworkState {
    bool connected{false};
    std::string status_message{"OFFLINE"};
};

enum class MetadataServiceKind {
    BuiltIn,
};

struct MetadataServiceState {
    MetadataServiceKind kind{MetadataServiceKind::BuiltIn};
    bool available{true};
    std::string display_name{"BUILT-IN"};
    std::string status{"OFFLINE"};
};

struct EqState {
    std::array<int, 10> bands{0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int selected_band{0};
    std::string preset_name{"FLAT"};
};

} // namespace lofibox::app
