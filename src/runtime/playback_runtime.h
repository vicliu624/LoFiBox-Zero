// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "application/app_service_registry.h"
#include "application/playback_command_service.h"
#include "runtime/runtime_snapshot.h"

namespace lofibox::runtime {

class PlaybackRuntime {
public:
    using RemoteTrackStarter = application::PlaybackCommandService::RemoteTrackStarter;

    explicit PlaybackRuntime(application::AppServiceRegistry services) noexcept;

    void setRemoteTrackStarter(RemoteTrackStarter starter);

    [[nodiscard]] bool playFirstAvailable() const;
    [[nodiscard]] bool startTrack(int track_id) const;
    void pause() const noexcept;
    void resume() const noexcept;
    void togglePlayPause() const noexcept;
    [[nodiscard]] PlaybackRuntimeSnapshot snapshot(std::uint64_t version) const;

private:
    application::AppServiceRegistry services_;
    RemoteTrackStarter remote_track_starter_{};
};

} // namespace lofibox::runtime
