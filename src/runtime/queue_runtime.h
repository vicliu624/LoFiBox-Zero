// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "application/app_service_registry.h"
#include "application/playback_command_service.h"
#include "runtime/runtime_snapshot.h"

namespace lofibox::runtime {

class QueueRuntime {
public:
    using RemoteTrackStarter = application::PlaybackCommandService::RemoteTrackStarter;

    explicit QueueRuntime(application::AppServiceRegistry services) noexcept;

    void setRemoteTrackStarter(RemoteTrackStarter starter);

    [[nodiscard]] bool step(int delta) const;
    void cycleMainMenuPlaybackMode() const;
    void toggleShuffle() const;
    void cycleRepeatMode() const noexcept;
    void setRepeatAll(bool enabled) const noexcept;
    void setRepeatOne(bool enabled) const noexcept;
    [[nodiscard]] QueueRuntimeSnapshot snapshot(std::uint64_t version) const;

private:
    application::AppServiceRegistry services_;
    RemoteTrackStarter remote_track_starter_{};
};

} // namespace lofibox::runtime
