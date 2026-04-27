// SPDX-License-Identifier: GPL-3.0-or-later

#include "playback/playback_stability_policy.h"

#include <cassert>

int main()
{
    lofibox::app::PlaybackStabilityPolicy policy{};
    lofibox::app::PlaybackStabilitySample starting{};
    starting.backend_starting = true;
    starting.position = std::chrono::milliseconds{10};
    starting.duration = std::chrono::seconds{180};
    assert(policy.suppressStartOrEndJitter(starting));

    lofibox::app::PlaybackStabilitySample ending{};
    ending.position = std::chrono::milliseconds{179900};
    ending.duration = std::chrono::seconds{180};
    assert(policy.suppressStartOrEndJitter(ending));

    lofibox::app::GaplessCrossfadePolicy gapless{};
    gapless.gapless_enabled = true;
    assert(policy.shouldPrepareNext(ending, gapless));

    lofibox::app::GaplessCrossfadePolicy crossfade{};
    crossfade.crossfade_enabled = true;
    crossfade.crossfade_duration = std::chrono::seconds{5};
    assert(policy.transitionLeadTime(crossfade) == std::chrono::milliseconds{5120});
    return 0;
}
