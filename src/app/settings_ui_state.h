// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <string>

namespace lofibox::app {

struct SettingsUiState {
    int sleep_timer_index{0};
    int backlight_index{1};
    int language_index{0};
    std::string sleep_timer{"OFF"};
    std::string screen_timeout{"OFF"};
};

} // namespace lofibox::app
