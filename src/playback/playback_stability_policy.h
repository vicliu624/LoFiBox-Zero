// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>

namespace lofibox::app {

struct GaplessCrossfadePolicy {
    bool gapless_enabled{true};
    bool crossfade_enabled{false};
    std::chrono::milliseconds crossfade_duration{std::chrono::milliseconds{0}};
    std::chrono::milliseconds transition_guard{std::chrono::milliseconds{120}};
};

struct PlaybackStabilitySample {
    std::chrono::milliseconds position{};
    std::chrono::milliseconds duration{};
    bool backend_starting{false};
    bool backend_finished{false};
    bool user_seeking{false};
};

class PlaybackStabilityPolicy {
public:
    [[nodiscard]] bool suppressStartOrEndJitter(const PlaybackStabilitySample& sample) const noexcept;
    [[nodiscard]] bool shouldPrepareNext(const PlaybackStabilitySample& sample, const GaplessCrossfadePolicy& policy) const noexcept;
    [[nodiscard]] std::chrono::milliseconds transitionLeadTime(const GaplessCrossfadePolicy& policy) const noexcept;
};

} // namespace lofibox::app
