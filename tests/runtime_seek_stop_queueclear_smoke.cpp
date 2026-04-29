// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <iostream>
#include <memory>
#include <utility>

#include "app/runtime_services.h"
#include "application/app_service_host.h"
#include "runtime/runtime_host.h"

namespace {

class RuntimeMutationAudioBackend final : public lofibox::app::AudioPlaybackBackend {
public:
    [[nodiscard]] bool available() const override { return true; }
    [[nodiscard]] std::string displayName() const override { return "RUNTIME-MUTATION-TEST"; }
    bool playFile(const std::filesystem::path&, double start_seconds) override
    {
        last_start_seconds = start_seconds;
        playing = true;
        return true;
    }
    void stop() override { playing = false; stopped = true; }
    [[nodiscard]] bool isPlaying() override { return playing; }
    [[nodiscard]] bool isFinished() override { return false; }

    double last_start_seconds{0.0};
    bool playing{false};
    bool stopped{false};
};

lofibox::runtime::RuntimeCommand command(lofibox::runtime::RuntimeCommandKind kind, lofibox::runtime::RuntimeCommandPayload payload = {})
{
    return lofibox::runtime::RuntimeCommand{kind, std::move(payload), lofibox::runtime::CommandOrigin::DirectTest};
}

} // namespace

int main()
{
    auto services = lofibox::app::withNullRuntimeServices();
    auto backend = std::make_shared<RuntimeMutationAudioBackend>();
    services.playback.audio_backend = backend;

    lofibox::application::AppServiceHost app_host{services};
    auto& model = app_host.controllers().library.mutableModel();
    model.tracks.push_back(lofibox::app::TrackRecord{71, std::filesystem::path{"one.mp3"}, "One", "Artist", "Album"});
    model.tracks.push_back(lofibox::app::TrackRecord{72, std::filesystem::path{"two.mp3"}, "Two", "Artist", "Album"});
    app_host.controllers().library.setSongsContextAll();

    lofibox::runtime::RuntimeHost runtime_host{app_host.registry()};
    if (!runtime_host.client().dispatch(command(
            lofibox::runtime::RuntimeCommandKind::PlaybackStartTrack,
            lofibox::runtime::RuntimeCommandPayload::startTrack(71))).applied) {
        std::cerr << "Expected start-track to prepare the seek/stop/queue-clear scenario.\n";
        return 1;
    }

    const auto seek = runtime_host.client().dispatch(command(
        lofibox::runtime::RuntimeCommandKind::PlaybackSeek,
        lofibox::runtime::RuntimeCommandPayload::seek(11.0)));
    if (!seek.accepted || !seek.applied || backend->last_start_seconds != 11.0) {
        std::cerr << "Expected PlaybackSeek contract command to be fully implemented.\n";
        return 1;
    }

    const auto clear = runtime_host.client().dispatch(command(lofibox::runtime::RuntimeCommandKind::QueueClear));
    const auto queue = runtime_host.client().query(lofibox::runtime::RuntimeQuery{lofibox::runtime::RuntimeQueryKind::QueueSnapshot});
    if (!clear.accepted || !clear.applied || !queue.queue.active_ids.empty()) {
        std::cerr << "Expected QueueClear contract command to empty the active queue.\n";
        return 1;
    }

    const auto stop = runtime_host.client().dispatch(command(lofibox::runtime::RuntimeCommandKind::PlaybackStop));
    const auto playback = runtime_host.client().query(lofibox::runtime::RuntimeQuery{lofibox::runtime::RuntimeQueryKind::PlaybackSnapshot});
    if (!stop.accepted || !stop.applied || playback.playback.status != lofibox::runtime::RuntimePlaybackStatus::Paused || !backend->stopped) {
        std::cerr << "Expected PlaybackStop contract command to stop the backend and leave runtime paused.\n";
        return 1;
    }

    return 0;
}
