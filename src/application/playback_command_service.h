// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>
#include <string>

#include "app/remote_media_services.h"
#include "playback/playback_controller.h"

namespace lofibox::audio::dsp {
struct DspChainProfile;
}

namespace lofibox::application {

class PlaybackCommandService {
public:
    using RemoteTrackStarter = app::PlaybackController::RemoteTrackStarter;

    PlaybackCommandService(app::PlaybackController& playback, app::LibraryController& library) noexcept;

    [[nodiscard]] const app::PlaybackSession& session() const noexcept;
    void update(double delta_seconds, const RemoteTrackStarter& remote_starter = {}) const;
    void setDspProfile(::lofibox::audio::dsp::DspChainProfile profile) const;

    [[nodiscard]] bool playFirstAvailable(const RemoteTrackStarter& remote_starter = {}) const;
    [[nodiscard]] bool startTrack(int track_id, const RemoteTrackStarter& remote_starter = {}) const;
    void prepareQueueForTrack(int track_id) const;
    void stepQueue(int delta, const RemoteTrackStarter& remote_starter = {}) const;
    void pause() const noexcept;
    void resume() const noexcept;
    void togglePlayPause() const noexcept;
    void setShuffleEnabled(bool enabled) const;
    void toggleShuffle() const;
    void setRepeatAll(bool enabled) const noexcept;
    void setRepeatOne(bool enabled) const noexcept;
    void cycleRepeatMode() const noexcept;
    void cycleMainMenuPlaybackMode() const;

    [[nodiscard]] bool startRemoteStream(const app::ResolvedRemoteStream& stream, const app::RemoteTrack& track, const std::string& source) const;
    [[nodiscard]] bool startRemoteLibraryTrack(
        const app::ResolvedRemoteStream& stream,
        app::TrackRecord& track,
        const app::RemoteServerProfile& profile,
        const app::RemoteTrack& remote_track,
        const std::string& source,
        bool cache_remote_facts) const;

private:
    app::PlaybackController& playback_;
    app::LibraryController& library_;
};

} // namespace lofibox::application
