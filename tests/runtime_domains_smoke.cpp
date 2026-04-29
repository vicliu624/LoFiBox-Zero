// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <iostream>
#include <memory>

#include "app/app_controller_set.h"
#include "app/runtime_services.h"
#include "runtime/eq_runtime.h"
#include "runtime/eq_runtime_state.h"
#include "runtime/playback_runtime.h"
#include "runtime/queue_runtime.h"
#include "runtime/remote_session_runtime.h"
#include "runtime/runtime_snapshot_assembler.h"
#include "runtime/settings_runtime.h"
#include "runtime/settings_runtime_state.h"

namespace {

class RuntimeDomainAudioBackend final : public lofibox::app::AudioPlaybackBackend {
public:
    [[nodiscard]] bool available() const override { return true; }
    [[nodiscard]] std::string displayName() const override { return "RUNTIME-DOMAIN-TEST"; }
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

} // namespace

int main()
{
    auto services = lofibox::app::withNullRuntimeServices();
    services.playback.audio_backend = std::make_shared<RuntimeDomainAudioBackend>();

    lofibox::app::AppControllerSet controllers{};
    controllers.bindServices(services);
    auto& model = controllers.library.mutableModel();
    model.tracks.push_back(lofibox::app::TrackRecord{21, std::filesystem::path{"one.mp3"}, "One", "Artist", "Album"});
    model.tracks.push_back(lofibox::app::TrackRecord{22, std::filesystem::path{"two.mp3"}, "Two", "Artist", "Album"});
    controllers.library.setSongsContextAll();

    auto registry = lofibox::application::AppServiceRegistry{controllers, services};
    lofibox::runtime::EqRuntimeState eq{};
    lofibox::runtime::SettingsRuntimeState settings_state{};
    lofibox::runtime::PlaybackRuntime playback{registry};
    lofibox::runtime::QueueRuntime queue{registry};
    lofibox::runtime::EqRuntime eq_runtime{registry, eq};
    lofibox::runtime::RemoteSessionRuntime remote{};
    lofibox::runtime::SettingsRuntime settings{settings_state};
    lofibox::runtime::RuntimeSnapshotAssembler assembler{};

    if (!playback.startTrack(21)) {
        std::cerr << "Expected PlaybackRuntime to own start-track mutation.\n";
        return 1;
    }
    auto playback_snapshot = playback.snapshot(1);
    if (playback_snapshot.current_track_id != 21 || playback_snapshot.title != "One") {
        std::cerr << "Expected PlaybackRuntime snapshot to expose current playback truth.\n";
        return 1;
    }

    if (!queue.step(1) || controllers.playback.session().current_track_id != 22) {
        std::cerr << "Expected QueueRuntime to own active queue stepping.\n";
        return 1;
    }
    if (!queue.jump(0) || controllers.playback.session().current_track_id != 21) {
        std::cerr << "Expected QueueRuntime to own queue jumping.\n";
        return 1;
    }
    auto queue_snapshot = queue.snapshot(2);
    if (queue_snapshot.active_ids.size() != 2U || queue_snapshot.active_index != 0) {
        std::cerr << "Expected QueueRuntime snapshot to expose active queue truth.\n";
        return 1;
    }

    if (!eq_runtime.setBand(3, -99)) {
        std::cerr << "Expected EqRuntime to accept a valid band mutation.\n";
        return 1;
    }
    auto eq_snapshot = eq_runtime.snapshot(3);
    if (eq_snapshot.bands[3] != -12 || eq_snapshot.preset_name != "CUSTOM" || !eq_snapshot.enabled) {
        std::cerr << "Expected EqRuntime to clamp and expose runtime EQ truth.\n";
        return 1;
    }

    lofibox::runtime::RemoteSessionSnapshot remote_snapshot{};
    remote_snapshot.profile_id = "domain-remote";
    remote_snapshot.source_label = "DOMAIN REMOTE";
    remote_snapshot.connection_status = "ONLINE";
    remote_snapshot.stream_resolved = true;
    remote_snapshot.seekable = true;
    remote.setSnapshot(remote_snapshot);
    if (!remote.reconnect()) {
        std::cerr << "Expected RemoteSessionRuntime to own live remote reconnect readiness.\n";
        return 1;
    }

    settings.applyLive("ALSA", "LAN_ONLY", "15M");
    if (settings.snapshot(4).output_mode != "ALSA" || settings.snapshot(4).sleep_timer != "15M") {
        std::cerr << "Expected SettingsRuntime to own live settings truth.\n";
        return 1;
    }

    const auto full = assembler.assemble(playback, queue, eq_runtime, remote, settings, 9);
    if (full.version != 9
        || full.playback.version != 9
        || full.queue.version != 9
        || full.eq.version != 9
        || full.remote.profile_id != "domain-remote"
        || full.settings.version != 9) {
        std::cerr << "Expected RuntimeSnapshotAssembler to compose all runtime domain snapshots.\n";
        return 1;
    }

    return 0;
}
