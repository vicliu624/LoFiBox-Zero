// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

#include "app/library_controller.h"
#include "app/playback_state.h"
#include "app/runtime_services.h"

namespace lofibox::app {

class PlaybackController {
public:
    ~PlaybackController();

    void setServices(RuntimeServices services);

    [[nodiscard]] const PlaybackSession& session() const noexcept;
    [[nodiscard]] PlaybackSession& mutableSession() noexcept;

    void update(double delta_seconds, LibraryController& library_controller);
    [[nodiscard]] bool startTrack(LibraryController& library_controller, int track_id);
    void stepQueue(LibraryController& library_controller, int delta);
    void pause() noexcept;
    void resume() noexcept;
    void togglePlayPause() noexcept;
    void setShuffleEnabled(bool enabled);
    void toggleShuffle();
    void setRepeatAll(bool enabled) noexcept;
    void setRepeatOne(bool enabled) noexcept;
    void cycleRepeatMode() noexcept;

private:
    void rebuildQueueForCurrentSongs(const LibraryController& library_controller, int selected_track_id);
    void rebuildActiveQueueForMode(std::optional<int> selected_track_id);
    [[nodiscard]] bool playQueueIndex(LibraryController& library_controller, int queue_index);
    [[nodiscard]] bool advanceAfterFinish(LibraryController& library_controller);
    void refreshArtwork(const LibraryController& library_controller, ArtworkReadMode mode = ArtworkReadMode::AllowOnline);
    void refreshMetadata(LibraryController& library_controller, MetadataReadMode mode = MetadataReadMode::AllowOnline);
    void requestEnrichment(const TrackRecord& track);
    void applyPendingEnrichments(LibraryController& library_controller);
    void synchronizeBackendState(LibraryController& library_controller);
    void updateVisualizationFrame();

    [[nodiscard]] static TrackMetadata metadataFromTrack(const TrackRecord& track);
    static void applyMetadataToTrack(TrackRecord& track, const TrackMetadata& metadata);

    QueueState queue_{};
    PlaybackSession session_{};
    RuntimeServices services_{};
    std::mutex enrichment_mutex_{};
    std::vector<PlaybackEnrichmentResult> pending_enrichments_{};
    std::vector<std::thread> enrichment_threads_{};
    std::uint64_t enrichment_generation_{0};
};

} // namespace lofibox::app
