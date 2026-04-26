// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/library_model.h"
#include "app/playback_state.h"

namespace lofibox::app {

class PlaybackSessionClock {
public:
    void resetForTrack(PlaybackSession& session) const noexcept;
    void advance(PlaybackSession& session, double delta_seconds) const noexcept;
    void clampToTrackDuration(PlaybackSession& session, const TrackRecord* track) const noexcept;
};

} // namespace lofibox::app
