// SPDX-License-Identifier: GPL-3.0-or-later

#include "playback/playback_stability_policy.h"

namespace lofibox::app {

bool PlaybackStabilityPolicy::suppressStartOrEndJitter(const PlaybackStabilitySample& sample) const noexcept
{
    if (sample.user_seeking) {
        return false;
    }
    if (sample.backend_starting && sample.position < std::chrono::milliseconds{250}) {
        return true;
    }
    if (sample.duration > std::chrono::milliseconds{0} && sample.duration - sample.position < std::chrono::milliseconds{250}) {
        return true;
    }
    return false;
}

bool PlaybackStabilityPolicy::shouldPrepareNext(const PlaybackStabilitySample& sample, const GaplessCrossfadePolicy& policy) const noexcept
{
    if (!policy.gapless_enabled && !policy.crossfade_enabled) {
        return false;
    }
    if (sample.duration <= std::chrono::milliseconds{0}) {
        return false;
    }
    return sample.duration - sample.position <= transitionLeadTime(policy);
}

std::chrono::milliseconds PlaybackStabilityPolicy::transitionLeadTime(const GaplessCrossfadePolicy& policy) const noexcept
{
    if (policy.crossfade_enabled) {
        return policy.crossfade_duration + policy.transition_guard;
    }
    return policy.transition_guard;
}

} // namespace lofibox::app
