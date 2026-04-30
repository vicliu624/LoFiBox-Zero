// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>

#include "app/library_controller.h"
#include "app/remote_media_services.h"
#include "playback/playback_enrichment_coordinator.h"
#include "playback/playback_runtime_coordinator.h"
#include "playback/playback_state.h"
#include "app/runtime_services.h"

namespace lofibox::app {

class PlaybackController {
public:
    using RemoteTrackStarter = std::function<bool(int)>;
    using PlaybackStartedRecorder = std::function<void(TrackRecord&)>;

    void setServices(RuntimeServices services);
    void setPlaybackStartedRecorder(PlaybackStartedRecorder recorder);
    void setDspProfile(::lofibox::audio::dsp::DspChainProfile profile);

    [[nodiscard]] const PlaybackSession& session() const noexcept;
    [[nodiscard]] PlaybackSession& mutableSession() noexcept;
    [[nodiscard]] const QueueState& queue() const noexcept;

    void update(double delta_seconds, LibraryController& library_controller);
    void update(double delta_seconds, LibraryController& library_controller, const RemoteTrackStarter& remote_starter);
    [[nodiscard]] bool startTrack(LibraryController& library_controller, int track_id);
    void prepareQueueForTrack(const LibraryController& library_controller, int track_id);
    [[nodiscard]] bool startRemoteStream(const ResolvedRemoteStream& stream, const RemoteTrack& track, const std::string& source);
    [[nodiscard]] bool startRemoteLibraryTrack(
        const ResolvedRemoteStream& stream,
        TrackRecord& track,
        const RemoteServerProfile& profile,
        const RemoteTrack& remote_track,
        const std::string& source,
        bool cache_remote_facts);
    [[nodiscard]] bool stepQueue(LibraryController& library_controller, int delta);
    [[nodiscard]] bool stepQueue(LibraryController& library_controller, int delta, const RemoteTrackStarter& remote_starter);
    [[nodiscard]] bool jumpQueue(LibraryController& library_controller, int queue_index);
    [[nodiscard]] bool jumpQueue(LibraryController& library_controller, int queue_index, const RemoteTrackStarter& remote_starter);
    void clearQueue() noexcept;
    void pause() noexcept;
    void resume() noexcept;
    void stop() noexcept;
    [[nodiscard]] bool seek(LibraryController& library_controller, double seconds);
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
    [[nodiscard]] bool playQueueIndex(LibraryController& library_controller, int queue_index, const RemoteTrackStarter& remote_starter);
    void refreshArtwork(const LibraryController& library_controller, ArtworkReadMode mode = ArtworkReadMode::AllowOnline);
    void refreshMetadata(LibraryController& library_controller, MetadataReadMode mode = MetadataReadMode::AllowOnline);
    void recordPlaybackStarted(TrackRecord& track);

    QueueState queue_{};
    PlaybackSession session_{};
    RuntimeServices services_{};
    PlaybackEnrichmentCoordinator enrichment_{};
    PlaybackRuntimeCoordinator runtime_{};
    PlaybackStartedRecorder playback_started_recorder_{};
};

} // namespace lofibox::app

