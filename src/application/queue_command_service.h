// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "application/playback_command_service.h"

namespace lofibox::application {

class QueueCommandService {
public:
    explicit QueueCommandService(PlaybackCommandService playback) noexcept;

    void step(int delta, const PlaybackCommandService::RemoteTrackStarter& remote_starter = {}) const;
    [[nodiscard]] bool jump(int queue_index, const PlaybackCommandService::RemoteTrackStarter& remote_starter = {}) const;
    void clear() const noexcept;
    void prepareForTrack(int track_id) const;

private:
    PlaybackCommandService playback_;
};

} // namespace lofibox::application
