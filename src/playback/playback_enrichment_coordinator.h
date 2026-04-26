// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

#include "app/library_controller.h"
#include "playback/playback_state.h"
#include "app/runtime_services.h"

namespace lofibox::app {

class PlaybackEnrichmentCoordinator {
public:
    ~PlaybackEnrichmentCoordinator();

    void setServices(RuntimeServices services);
    void request(const TrackRecord& track);
    void applyPending(LibraryController& library_controller, PlaybackSession& session);

    [[nodiscard]] static TrackMetadata metadataFromTrack(const TrackRecord& track);
    static void applyMetadataToTrack(TrackRecord& track, const TrackMetadata& metadata);

private:
    RuntimeServices services_{};
    std::mutex mutex_{};
    std::vector<PlaybackEnrichmentResult> pending_results_{};
    std::vector<std::thread> threads_{};
    std::uint64_t generation_{0};
};

} // namespace lofibox::app

