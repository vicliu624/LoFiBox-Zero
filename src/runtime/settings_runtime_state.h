// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

namespace lofibox::runtime {

struct SettingsRuntimeState {
    std::string output_mode{"DEFAULT"};
    std::string network_policy{"DEFAULT"};
    std::string sleep_timer{"OFF"};
    bool shutdown_requested{false};
    bool reload_requested{false};
};

} // namespace lofibox::runtime
