// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <iostream>
#include <memory>

#include "app/runtime_services.h"
#include "application/app_service_host.h"
#include "runtime/runtime_host.h"
#include "runtime/unix_socket_runtime_transport.h"

namespace {

class RuntimeSocketAudioBackend final : public lofibox::app::AudioPlaybackBackend {
public:
    [[nodiscard]] bool available() const override { return true; }
    [[nodiscard]] std::string displayName() const override { return "RUNTIME-SOCKET-TEST"; }
    bool playFile(const std::filesystem::path&, double) override
    {
        playing = true;
        return true;
    }
    void stop() override { playing = false; }
    [[nodiscard]] bool isPlaying() override { return playing; }
    [[nodiscard]] bool isFinished() override { return false; }

    bool playing{false};
};

} // namespace

int main()
{
#if defined(__unix__) || defined(__APPLE__)
    auto services = lofibox::app::withNullRuntimeServices();
    services.playback.audio_backend = std::make_shared<RuntimeSocketAudioBackend>();

    lofibox::application::AppServiceHost app_host{services};
    app_host.controllers().library.mutableModel().tracks.push_back(
        lofibox::app::TrackRecord{51, std::filesystem::path{"socket.mp3"}, "Socket", "Artist", "Album"});
    app_host.controllers().library.setSongsContextAll();

    const auto socket_path = std::filesystem::temp_directory_path() / "lofibox-runtime-transport-smoke.sock";
    std::error_code ec{};
    std::filesystem::remove(socket_path, ec);

    lofibox::runtime::RuntimeHost runtime_host{app_host.registry()};
    if (!runtime_host.startExternalTransport(socket_path)) {
        std::cerr << "Expected RuntimeHost to start external socket transport: "
                  << runtime_host.externalTransportError() << '\n';
        return 1;
    }

    lofibox::runtime::UnixSocketRuntimeCommandClient client{socket_path};
    const auto result = client.dispatch(lofibox::runtime::RuntimeCommand{
        lofibox::runtime::RuntimeCommandKind::PlaybackStartTrack,
        lofibox::runtime::RuntimeCommandPayload::startTrack(51),
        lofibox::runtime::CommandOrigin::RuntimeCli,
        "socket-start"});
    if (!result.accepted || !result.applied || result.origin != lofibox::runtime::CommandOrigin::RuntimeCli) {
        std::cerr << "Expected socket runtime client to apply a playback command through transport.\n";
        return 1;
    }

    const auto snapshot = client.query(lofibox::runtime::RuntimeQuery{
        lofibox::runtime::RuntimeQueryKind::PlaybackSnapshot,
        lofibox::runtime::CommandOrigin::RuntimeCli,
        "socket-playback"});
    if (snapshot.playback.current_track_id != 51 || snapshot.playback.title != "Socket") {
        std::cerr << "Expected socket runtime query to return the hosted playback snapshot.\n";
        return 1;
    }

    runtime_host.stopExternalTransport();
    std::filesystem::remove(socket_path, ec);
#endif
    return 0;
}
