// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include "runtime/settings_runtime_state.h"
#include "runtime/runtime_snapshot.h"

namespace lofibox::runtime {

class SettingsRuntime {
public:
    explicit SettingsRuntime(SettingsRuntimeState& state) noexcept;

    bool applyLive(std::string output_mode, std::string network_policy, std::string sleep_timer,
                   std::string screen_timeout = {},
                   std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now());
    [[nodiscard]] bool sleepTimerExpired(std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now()) const noexcept;
    [[nodiscard]] std::chrono::seconds screenTimeout() const noexcept;
    void requestShutdown() noexcept;
    void requestReload() noexcept;
    [[nodiscard]] bool shutdownRequested() const noexcept;
    [[nodiscard]] bool reloadRequested() const noexcept;
    [[nodiscard]] SettingsRuntimeSnapshot snapshot(std::uint64_t version) const noexcept;

private:
    SettingsRuntimeState& state_;
};

} // namespace lofibox::runtime
