// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <iostream>
#include <memory>

#include "app/runtime_services.h"
#include "application/app_service_host.h"
#include "runtime/runtime_host.h"

namespace {

class RuntimeHostAudioBackend final : public lofibox::app::AudioPlaybackBackend {
public:
    [[nodiscard]] bool available() const override { return true; }
    [[nodiscard]] std::string displayName() const override { return "RUNTIME-HOST-TEST"; }
    bool playFile(const std::filesystem::path& path, double) override
    {
        played_path = path;
        playing = true;
        return true;
    }
    bool playUri(const std::string& uri, double) override
    {
        played_uri = uri;
        playing = true;
        return true;
    }
    void stop() override { playing = false; }
    [[nodiscard]] bool isPlaying() override { return playing; }
    [[nodiscard]] bool isFinished() override { return false; }

    std::filesystem::path played_path{};
    std::string played_uri{};
    bool playing{false};
};

} // namespace

int main()
{
    auto services = lofibox::app::withNullRuntimeServices();
    auto backend = std::make_shared<RuntimeHostAudioBackend>();
    services.playback.audio_backend = backend;

    lofibox::application::AppServiceHost app_host{services};
    app_host.controllers().library.mutableModel().tracks.push_back(
        lofibox::app::TrackRecord{41, std::filesystem::path{"host.mp3"}, "Host", "Artist", "Album"});
    app_host.controllers().library.setSongsContextAll();

    lofibox::runtime::RuntimeHost runtime_host{app_host.registry()};
    const auto started = runtime_host.client().dispatch(lofibox::runtime::RuntimeCommand{
        lofibox::runtime::RuntimeCommandKind::PlaybackStartTrack,
        lofibox::runtime::RuntimeCommandPayload::startTrack(41),
        lofibox::runtime::CommandOrigin::DirectTest,
        "host-start"});
    if (!started.accepted || !started.applied || backend->played_path != "host.mp3") {
        std::cerr << "Expected RuntimeHost to own live playback dispatch outside AppRuntimeContext.\n";
        return 1;
    }

    runtime_host.tick(0.25);
    const auto snapshot = runtime_host.client().query(lofibox::runtime::RuntimeQuery{
        lofibox::runtime::RuntimeQueryKind::FullSnapshot,
        lofibox::runtime::CommandOrigin::DirectTest,
        "host-snapshot"});
    if (snapshot.playback.current_track_id != 41 || snapshot.version != 1U) {
        std::cerr << "Expected RuntimeHost client snapshot to expose the hosted runtime state.\n";
        return 1;
    }

    return 0;
}
