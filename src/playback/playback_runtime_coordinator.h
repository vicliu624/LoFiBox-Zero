// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>
#include <filesystem>

#include "app/library_controller.h"
#include "playback/playback_completion_policy.h"
#include "playback/playback_session_clock.h"
#include "audio/audio_pipeline_controller.h"

namespace lofibox::app {

class PlaybackRuntimeCoordinator {
public:
    using PlayIndexCallback = std::function<bool(int)>;

    void setServices(RuntimeServices* services) noexcept;

    void beginTrack(PlaybackSession& session) const noexcept;
    [[nodiscard]] bool startBackend(const std::filesystem::path& path, PlaybackSession& session);
    void pause(PlaybackSession& session) noexcept;
    void resume(PlaybackSession& session) noexcept;
    void stepQueue(const QueueState& queue, const PlaybackSession& session, int delta, const PlayIndexCallback& play_index) const;
    [[nodiscard]] bool advanceAfterFinish(const QueueState& queue, const PlaybackSession& session, const PlayIndexCallback& play_index) const;
    void tick(PlaybackSession& session, const QueueState& queue, const LibraryController& library_controller, double delta_seconds, const PlayIndexCallback& play_index);

private:
    PlaybackSessionClock clock_{};
    audio::AudioPipelineController audio_pipeline_{};
    PlaybackCompletionPolicy completion_policy_{};
};

} // namespace lofibox::app

