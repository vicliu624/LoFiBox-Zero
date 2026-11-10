// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>
#include <optional>
#include <string>

namespace lofibox::runtime {

struct SettingsRuntimeState {
    std::string output_mode{"DEFAULT"};
    std::string network_policy{"DEFAULT"};
    std::string sleep_timer{"OFF"};
    std::string screen_timeout{"OFF"};
    std::chrono::seconds screen_timeout_duration{};
    std::optional<std::chrono::steady_clock::time_point> sleep_deadline{};
    bool screen_timeout_supported{true};
    bool shutdown_requested{false};
    bool reload_requested{false};
};

} // namespace lofibox::runtime
