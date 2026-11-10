// SPDX-License-Identifier: GPL-3.0-or-later

#include "runtime/settings_runtime.h"

#include <utility>
#include "core/session_timer.h"

namespace lofibox::runtime {

SettingsRuntime::SettingsRuntime(SettingsRuntimeState& state) noexcept
    : state_(state)
{
}

bool SettingsRuntime::applyLive(std::string output_mode, std::string network_policy, std::string sleep_timer,
                                std::string screen_timeout, std::chrono::steady_clock::time_point now)
{
    const auto sleep = core::parseTimerDuration(sleep_timer);
    const auto screen = core::parseTimerDuration(screen_timeout);
    // Validate the entire update before mutating any state.
    if ((!sleep_timer.empty() && !sleep) || (!screen_timeout.empty() && !screen)) return false;
    if (screen && *screen > std::chrono::seconds::zero() && !state_.screen_timeout_supported) return false;
    if (!output_mode.empty()) {
        state_.output_mode = std::move(output_mode);
    }
    if (!network_policy.empty()) {
        state_.network_policy = std::move(network_policy);
    }
    if (!sleep_timer.empty()) {
        state_.sleep_timer = std::move(sleep_timer);
        state_.sleep_deadline = *sleep > std::chrono::seconds::zero()
            ? std::make_optional(now + *sleep) : std::nullopt;
    }
    if (!screen_timeout.empty()) {
        state_.screen_timeout = std::move(screen_timeout);
        state_.screen_timeout_duration = *screen;
    }
    return true;
}

bool SettingsRuntime::sleepTimerExpired(std::chrono::steady_clock::time_point now) const noexcept
{
    return state_.sleep_deadline && now >= *state_.sleep_deadline;
}

std::chrono::seconds SettingsRuntime::screenTimeout() const noexcept
{
    return state_.screen_timeout_duration;
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
    result.screen_timeout = state_.screen_timeout;
    result.screen_timeout_supported = state_.screen_timeout_supported;
    result.version = version;
    return result;
}

} // namespace lofibox::runtime
