// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "application/app_service_registry.h"
#include "runtime/runtime_command.h"
#include "runtime/runtime_snapshot.h"

namespace lofibox::runtime {

class PlaybackRuntime {
public:
    explicit PlaybackRuntime(application::AppServiceRegistry services) noexcept;

    void update(double delta_seconds) const;
    [[nodiscard]] bool playFirstAvailable() const;
    [[nodiscard]] bool startTrack(int track_id) const;
    [[nodiscard]] bool startRemoteStream(const RemotePlayResolvedStreamPayload& payload) const;
    [[nodiscard]] bool startRemoteLibraryTrack(const RemotePlayResolvedLibraryTrackPayload& payload) const;
    void stop() const noexcept;
    [[nodiscard]] bool seek(double seconds) const;
    void pause() const noexcept;
    [[nodiscard]] bool resume() const;
    [[nodiscard]] bool togglePlayPause() const;
    [[nodiscard]] PlaybackRuntimeSnapshot snapshot(std::uint64_t version) const;

private:
    application::AppServiceRegistry services_;
};

} // namespace lofibox::runtime
