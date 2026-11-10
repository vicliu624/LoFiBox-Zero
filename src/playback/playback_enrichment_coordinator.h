// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <stop_token>
#include <string>
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
    void requestRemote(
        const RemoteServerProfile& profile,
        const RemoteTrack& remote_track,
        int track_id,
        std::string stream_uri,
        bool cache_remote_facts);
    void applyPending(LibraryController& library_controller, PlaybackSession& session);

    [[nodiscard]] static TrackMetadata metadataFromTrack(const TrackRecord& track);
    static void applyMetadataToTrack(TrackRecord& track, const TrackMetadata& metadata);

private:
    RuntimeServices services_{};
    std::mutex mutex_{};
    std::vector<PlaybackEnrichmentResult> pending_results_{};
    void enqueue(std::function<void(std::stop_token)> job);
    void runWorker();
    std::condition_variable work_ready_{};
    std::function<void(std::stop_token)> pending_job_{};
    std::thread worker_{};
    std::stop_source active_cancellation_{};
    bool stopping_{false};
    std::uint64_t generation_{0};
};

} // namespace lofibox::app

