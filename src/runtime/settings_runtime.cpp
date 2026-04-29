// SPDX-License-Identifier: GPL-3.0-or-later

#include "runtime/settings_runtime.h"

#include <utility>

namespace lofibox::runtime {

SettingsRuntime::SettingsRuntime(SettingsRuntimeState& state) noexcept
    : state_(state)
{
}

void SettingsRuntime::applyLive(std::string output_mode, std::string network_policy, std::string sleep_timer)
{
    if (!output_mode.empty()) {
        state_.output_mode = std::move(output_mode);
    }
    if (!network_policy.empty()) {
        state_.network_policy = std::move(network_policy);
    }
    if (!sleep_timer.empty()) {
        state_.sleep_timer = std::move(sleep_timer);
    }
}

void SettingsRuntime::requestShutdown() noexcept
{
    state_.shutdown_requested = true;
}

void SettingsRuntime::requestReload() noexcept
{
    state_.reload_requested = true;
}

bool SettingsRuntime::shutdownRequested() const noexcept
{
    return state_.shutdown_requested;
}

bool SettingsRuntime::reloadRequested() const noexcept
{
    return state_.reload_requested;
}

SettingsRuntimeSnapshot SettingsRuntime::snapshot(std::uint64_t version) const noexcept
{
    SettingsRuntimeSnapshot result{};
    result.output_mode = state_.output_mode;
    result.network_policy = state_.network_policy;
    result.sleep_timer = state_.sleep_timer;
    result.version = version;
    return result;
}

} // namespace lofibox::runtime
