// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/library_mutation_service.h"

#include <chrono>

namespace lofibox::app {

void LibraryMutationService::recordPlaybackStarted(TrackRecord& track) const
{
    ++track.play_count;
    track.last_played = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

} // namespace lofibox::app
