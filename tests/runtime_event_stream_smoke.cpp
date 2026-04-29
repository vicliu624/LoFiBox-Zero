// SPDX-License-Identifier: GPL-3.0-or-later

#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <thread>

#include "app/runtime_services.h"
#include "application/app_service_host.h"
#include "runtime/runtime_host.h"
#include "runtime/unix_socket_runtime_transport.h"

namespace {

class EventStreamAudioBackend final : public lofibox::app::AudioPlaybackBackend {
public:
    [[nodiscard]] bool available() const override { return true; }
    [[nodiscard]] std::string displayName() const override { return "EVENT-STREAM-TEST"; }
    bool playFile(const std::filesystem::path&, double) override
    {
        playing = true;
        return true;
    }
    void stop() override { playing = false; }
    [[nodiscard]] bool isPlaying() override { return playing; }
    [[nodiscard]] bool isFinished() override { return false; }
    [[nodiscard]] lofibox::app::AudioVisualizationFrame visualizationFrame() const override
    {
        lofibox::app::AudioVisualizationFrame frame{};
        frame.available = true;
        frame.bands = {0.1F, 0.2F, 0.6F, 0.9F, 0.4F, 0.2F, 0.1F, 0.2F, 0.3F, 0.4F};
        return frame;
    }

    bool playing{false};
};

} // namespace

int main()
{
#if defined(__unix__) || defined(__APPLE__)
    auto services = lofibox::app::withNullRuntimeServices();
    services.playback.audio_backend = std::make_shared<EventStreamAudioBackend>();
    lofibox::application::AppServiceHost app_host{services};
    app_host.controllers().library.mutableModel().tracks.push_back(
        lofibox::app::TrackRecord{61, std::filesystem::path{"event.mp3"}, "Event", "Artist", "Album"});
    app_host.controllers().library.setSongsContextAll();

    const auto socket_path = std::filesystem::temp_directory_path() / "lofibox-runtime-event-stream-smoke.sock";
    std::error_code ec{};
    std::filesystem::remove(socket_path, ec);

    lofibox::runtime::RuntimeHost runtime_host{app_host.registry()};
    if (!runtime_host.startExternalTransport(socket_path)) {
        std::cerr << "Expected RuntimeHost to start external socket transport: "
                  << runtime_host.externalTransportError() << '\n';
        return 1;
    }

    lofibox::runtime::UnixSocketRuntimeEventStream stream{socket_path};
    lofibox::runtime::RuntimeEventStreamRequest request{};
    request.heartbeat_ms = 100;
    if (!stream.connect(request)) {
        std::cerr << "Expected event stream to connect: " << stream.lastError() << '\n';
        return 1;
    }

    auto first = stream.next(std::chrono::seconds{2});
    if (!first || first->kind != lofibox::runtime::RuntimeEventKind::RuntimeConnected) {
        std::cerr << "Expected initial runtime.connected event.\n";
        return 1;
    }
    auto initial_snapshot = stream.next(std::chrono::seconds{2});
    if (!initial_snapshot || initial_snapshot->kind != lofibox::runtime::RuntimeEventKind::RuntimeSnapshot) {
        std::cerr << "Expected initial runtime.snapshot event.\n";
        return 1;
    }

    lofibox::runtime::UnixSocketRuntimeCommandClient client{socket_path};
    const auto result = client.dispatch(lofibox::runtime::RuntimeCommand{
        lofibox::runtime::RuntimeCommandKind::PlaybackStartTrack,
        lofibox::runtime::RuntimeCommandPayload::startTrack(61),
        lofibox::runtime::CommandOrigin::RuntimeCli,
        "event-stream-start"});
    if (!result.accepted || !result.applied) {
        std::cerr << "Expected command client to remain usable while event stream is connected.\n";
        return 1;
    }

    bool saw_playback = false;
    bool saw_snapshot = true;
    for (int attempt = 0; attempt < 20 && !saw_playback; ++attempt) {
        runtime_host.tick(0.10);
        if (auto event = stream.next(std::chrono::milliseconds{250})) {
            saw_snapshot = saw_snapshot || event->kind == lofibox::runtime::RuntimeEventKind::RuntimeSnapshot;
            saw_playback = event->kind == lofibox::runtime::RuntimeEventKind::PlaybackChanged
                || event->kind == lofibox::runtime::RuntimeEventKind::PlaybackProgress
                || event->kind == lofibox::runtime::RuntimeEventKind::VisualizationFrame
                || event->kind == lofibox::runtime::RuntimeEventKind::CreatorChanged;
            if (saw_playback && event->snapshot.playback.title != "Event") {
                std::cerr << "Expected streamed event to carry the updated runtime snapshot.\n";
                return 1;
            }
        }
    }
    if (!saw_snapshot || !saw_playback) {
        std::cerr << "Expected event stream to push snapshot and playback/visualization events.\n";
        return 1;
    }

    stream.close();
    runtime_host.stopExternalTransport();
    std::filesystem::remove(socket_path, ec);
#endif
    return 0;
}
