// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include "app/eq_ui_state.h"
#include "app/settings_ui_state.h"

namespace lofibox::app {

using SettingsState = SettingsUiState;

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

using EqState = EqUiState;

} // namespace lofibox::app
