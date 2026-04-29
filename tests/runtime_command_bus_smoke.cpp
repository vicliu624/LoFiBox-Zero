// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <iostream>
#include <memory>

#include "app/app_controller_set.h"
#include "app/runtime_services.h"
#include "runtime/eq_runtime_state.h"
#include "runtime/runtime_command_bus.h"
#include "runtime/runtime_session_facade.h"
#include "runtime/settings_runtime_state.h"

namespace {

class RuntimeBusAudioBackend final : public lofibox::app::AudioPlaybackBackend {
public:
    [[nodiscard]] bool available() const override { return true; }
    [[nodiscard]] std::string displayName() const override { return "RUNTIME-BUS-TEST"; }
    bool playFile(const std::filesystem::path& path, double) override
    {
        last_path = path;
        playing = true;
        paused = false;
        return true;
    }
    void stop() override { playing = false; }
    void pause() override { paused = true; }
    void resume() override { paused = false; playing = true; }
    [[nodiscard]] bool isPlaying() override { return playing && !paused; }
    [[nodiscard]] bool isFinished() override { return false; }

    std::filesystem::path last_path{};
    bool playing{false};
    bool paused{false};
};

lofibox::runtime::RuntimeCommand command(lofibox::runtime::RuntimeCommandKind kind)
{
    return lofibox::runtime::RuntimeCommand{kind, {}, lofibox::runtime::CommandOrigin::DirectTest};
}

} // namespace

int main()
{
    auto services = lofibox::app::withNullRuntimeServices();
    auto backend = std::make_shared<RuntimeBusAudioBackend>();
    services.playback.audio_backend = backend;

    lofibox::app::AppControllerSet controllers{};
    controllers.bindServices(services);
    auto& model = controllers.library.mutableModel();
    model.tracks.push_back(lofibox::app::TrackRecord{11, std::filesystem::path{"alpha.mp3"}, "Alpha", "Artist", "Album"});
    model.tracks.push_back(lofibox::app::TrackRecord{12, std::filesystem::path{"beta.mp3"}, "Beta", "Artist", "Album"});
    controllers.library.setSongsContextAll();

    lofibox::runtime::EqRuntimeState eq{};
    lofibox::runtime::SettingsRuntimeState settings{};
    lofibox::runtime::RuntimeSessionFacade session{
        lofibox::application::AppServiceRegistry{controllers, services},
        eq,
        settings};
    lofibox::runtime::RemoteSessionSnapshot remote_snapshot{};
    remote_snapshot.profile_id = "remote-a";
    remote_snapshot.source_label = "REMOTE A";
    remote_snapshot.connection_status = "ONLINE";
    remote_snapshot.stream_resolved = true;
    remote_snapshot.buffer_state = "READY";
    session.remote().setSnapshot(remote_snapshot);

    lofibox::runtime::RuntimeCommandBus bus{session};

    auto result = bus.dispatch(lofibox::runtime::RuntimeCommand{
        lofibox::runtime::RuntimeCommandKind::PlaybackStartTrack,
        lofibox::runtime::RuntimeCommandPayload::startTrack(11),
        lofibox::runtime::CommandOrigin::DirectTest,
        "start-alpha"});
    if (!result.accepted
        || !result.applied
        || result.origin != lofibox::runtime::CommandOrigin::DirectTest
        || result.correlation_id != "start-alpha"
        || result.version_before_apply != 0U
        || result.version_after_apply != 1U) {
        std::cerr << "Expected runtime bus to accept and apply a start-track command with a version.\n";
        return 1;
    }
    auto snapshot = bus.snapshot();
    if (snapshot.playback.status != lofibox::runtime::RuntimePlaybackStatus::Playing
        || snapshot.playback.current_track_id != 11
        || snapshot.playback.title != "Alpha"
        || snapshot.queue.active_ids.size() != 2U) {
        std::cerr << "Expected runtime playback and queue snapshots to reflect the active track.\n";
        return 1;
    }

    result = bus.dispatch(lofibox::runtime::RuntimeCommand{
        lofibox::runtime::RuntimeCommandKind::QueueStep,
        lofibox::runtime::RuntimeCommandPayload::queueStep(1),
        lofibox::runtime::CommandOrigin::DirectTest});
    if (!result.applied || controllers.playback.session().current_track_id != 12) {
        std::cerr << "Expected queue stepping to mutate playback through the runtime bus.\n";
        return 1;
    }

    result = bus.dispatch(lofibox::runtime::RuntimeCommand{
        lofibox::runtime::RuntimeCommandKind::EqAdjustBand,
        lofibox::runtime::RuntimeCommandPayload::eqAdjustBand(0, 20),
        lofibox::runtime::CommandOrigin::DirectTest});
    snapshot = bus.query(lofibox::runtime::RuntimeQuery{lofibox::runtime::RuntimeQueryKind::EqSnapshot});
    if (!result.applied || snapshot.eq.bands[0] != 12 || snapshot.eq.preset_name != "CUSTOM" || !snapshot.eq.enabled) {
        std::cerr << "Expected EQ band adjustment to clamp, mark custom, and expose an EQ snapshot.\n";
        return 1;
    }

    result = bus.dispatch(lofibox::runtime::RuntimeCommand{
        lofibox::runtime::RuntimeCommandKind::EqApplyPreset,
        lofibox::runtime::RuntimeCommandPayload::eqApplyPreset("FLAT"),
        lofibox::runtime::CommandOrigin::DirectTest});
    if (!result.applied) {
        std::cerr << "Expected EQ flat preset to be applied through the runtime bus.\n";
        return 1;
    }

    result = bus.dispatch(lofibox::runtime::RuntimeCommand{
        lofibox::runtime::RuntimeCommandKind::EqCyclePreset,
        lofibox::runtime::RuntimeCommandPayload::eqCyclePreset(1),
        lofibox::runtime::CommandOrigin::DirectTest});
    snapshot = bus.snapshot();
    if (!result.applied || snapshot.eq.preset_name != "BASS BOOST" || snapshot.eq.bands[0] != 6 || snapshot.remote.source_label != "REMOTE A") {
        std::cerr << "Expected EQ preset and remote-session snapshots to be structured runtime truth.\n";
        return 1;
    }

    result = bus.dispatch(lofibox::runtime::RuntimeCommand{
        lofibox::runtime::RuntimeCommandKind::PlaybackSeek,
        lofibox::runtime::RuntimeCommandPayload::seek(1.0),
        lofibox::runtime::CommandOrigin::DirectTest});
    if (!result.accepted || !result.applied || result.code != "PLAYBACK_SEEK") {
        std::cerr << "Expected transport-neutral seek command to be implemented by the runtime bus.\n";
        return 1;
    }

    result = bus.dispatch(command(lofibox::runtime::RuntimeCommandKind::QueueClear));
    snapshot = bus.snapshot();
    if (!result.accepted || !result.applied || !snapshot.queue.active_ids.empty()) {
        std::cerr << "Expected queue clear command to be implemented by the runtime bus.\n";
        return 1;
    }

    return 0;
}
