// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

#include "runtime/settings_runtime_state.h"
#include "runtime/runtime_snapshot.h"

namespace lofibox::runtime {

class SettingsRuntime {
public:
    explicit SettingsRuntime(SettingsRuntimeState& state) noexcept;

    void applyLive(std::string output_mode, std::string network_policy, std::string sleep_timer);
    void requestShutdown() noexcept;
    void requestReload() noexcept;
    [[nodiscard]] bool shutdownRequested() const noexcept;
    [[nodiscard]] bool reloadRequested() const noexcept;
    [[nodiscard]] SettingsRuntimeSnapshot snapshot(std::uint64_t version) const noexcept;

private:
    SettingsRuntimeState& state_;
};

} // namespace lofibox::runtime
