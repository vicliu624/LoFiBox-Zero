// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <iostream>
#include <memory>
#include <atomic>
#include <chrono>
#include <thread>

#include "app/runtime_services.h"
#include "application/app_service_host.h"
#include "runtime/runtime_host.h"

namespace {

class RuntimeHostAudioBackend final : public lofibox::app::AudioPlaybackBackend {
public:
    struct EntryGuard {
        explicit EntryGuard(RuntimeHostAudioBackend& owner_in)
            : owner(owner_in)
        {
            if (owner.active_backend_entries.fetch_add(1) > 0) {
                owner.overlapped_backend_entries = true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{2});
        }

        ~EntryGuard()
        {
            owner.active_backend_entries.fetch_sub(1);
        }

        RuntimeHostAudioBackend& owner;
    };

    [[nodiscard]] bool available() const override { return true; }
    [[nodiscard]] std::string displayName() const override { return "RUNTIME-HOST-TEST"; }
    bool playFile(const std::filesystem::path& path, double) override
    {
        EntryGuard guard{*this};
        played_path = path;
        playing = true;
        return true;
    }
    bool playUri(const std::string& uri, double) override
    {
        EntryGuard guard{*this};
        played_uri = uri;
        playing = true;
        return true;
    }
    void stop() override { playing = false; }
    [[nodiscard]] bool isPlaying() override { return playing; }
    [[nodiscard]] bool isFinished() override { return false; }
    [[nodiscard]] lofibox::app::AudioPlaybackState state() override
    {
        EntryGuard guard{*this};
        return playing ? lofibox::app::AudioPlaybackState::Playing : lofibox::app::AudioPlaybackState::Idle;
    }

    std::filesystem::path played_path{};
    std::string played_uri{};
    bool playing{false};
    std::atomic<int> active_backend_entries{0};
    std::atomic_bool overlapped_backend_entries{false};
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

    std::thread ticker{[&runtime_host]() {
        for (int index = 0; index < 80; ++index) {
            runtime_host.tick(0.01);
        }
    }};
    std::thread dispatcher{[&runtime_host]() {
        for (int index = 0; index < 20; ++index) {
            (void)runtime_host.client().dispatch(lofibox::runtime::RuntimeCommand{
                lofibox::runtime::RuntimeCommandKind::PlaybackStartTrack,
                lofibox::runtime::RuntimeCommandPayload::startTrack(41),
                lofibox::runtime::CommandOrigin::DirectTest});
        }
    }};
    ticker.join();
    dispatcher.join();
    if (backend->overlapped_backend_entries) {
        std::cerr << "Expected RuntimeHost tick and external commands to be serialized through the runtime bus.\n";
        return 1;
    }

    return 0;
}
