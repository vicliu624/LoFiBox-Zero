// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>
#include <string_view>

#include "app/app_state.h"
#include "application/app_service_registry.h"
#include "application/playback_command_service.h"
#include "runtime/runtime_snapshot.h"

namespace lofibox::runtime {

class RuntimeSessionFacade {
public:
    using RemoteTrackStarter = application::PlaybackCommandService::RemoteTrackStarter;
    using ActiveRemoteStreamStarter = std::function<bool()>;
    using RemoteSessionSnapshotProvider = std::function<RemoteSessionSnapshot()>;

    RuntimeSessionFacade(application::AppServiceRegistry services, app::EqState& eq) noexcept;

    void setRemoteTrackStarter(RemoteTrackStarter starter);
    void setActiveRemoteStreamStarter(ActiveRemoteStreamStarter starter);
    void setRemoteSessionSnapshotProvider(RemoteSessionSnapshotProvider provider);

    [[nodiscard]] bool playFirstAvailable() const;
    [[nodiscard]] bool startTrack(int track_id) const;
    void pause() const noexcept;
    void resume() const noexcept;
    void togglePlayPause() const noexcept;
    [[nodiscard]] bool stepQueue(int delta) const;
    void cycleMainMenuPlaybackMode() const;
    void toggleShuffle() const;
    void cycleRepeatMode() const noexcept;
    void setRepeatAll(bool enabled) const noexcept;
    void setRepeatOne(bool enabled) const noexcept;
    [[nodiscard]] bool startActiveRemoteStream() const;

    void setEqEnabled(bool enabled);
    [[nodiscard]] bool setEqBand(int band_index, int gain_db);
    [[nodiscard]] bool adjustEqBand(int band_index, int delta_db);
    [[nodiscard]] bool applyEqPreset(std::string_view preset_name);
    [[nodiscard]] bool cycleEqPreset(int delta);
    void resetEq();

    [[nodiscard]] RuntimeSnapshot snapshot(std::uint64_t version) const;
    [[nodiscard]] PlaybackRuntimeSnapshot playbackSnapshot(std::uint64_t version) const;
    [[nodiscard]] QueueRuntimeSnapshot queueSnapshot(std::uint64_t version) const;
    [[nodiscard]] EqRuntimeSnapshot eqSnapshot(std::uint64_t version) const;
    [[nodiscard]] RemoteSessionSnapshot remoteSessionSnapshot(std::uint64_t version) const;

private:
    void applyEqToPlayback() const;

    application::AppServiceRegistry services_;
    app::EqState& eq_;
    RemoteTrackStarter remote_track_starter_{};
    ActiveRemoteStreamStarter active_remote_stream_starter_{};
    RemoteSessionSnapshotProvider remote_session_snapshot_provider_{};
};

} // namespace lofibox::runtime
