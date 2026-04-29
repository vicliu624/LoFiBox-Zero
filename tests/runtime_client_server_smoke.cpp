// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <iostream>
#include <memory>

#include "app/app_controller_set.h"
#include "app/runtime_services.h"
#include "runtime/in_process_runtime_client.h"
#include "runtime/runtime_command_bus.h"
#include "runtime/runtime_command_server.h"
#include "runtime/runtime_session_facade.h"
#include "runtime/runtime_transport.h"

namespace {

class RuntimeClientAudioBackend final : public lofibox::app::AudioPlaybackBackend {
public:
    [[nodiscard]] bool available() const override { return true; }
    [[nodiscard]] std::string displayName() const override { return "RUNTIME-CLIENT-TEST"; }
    bool playFile(const std::filesystem::path&, double) override
    {
        playing = true;
        paused = false;
        return true;
    }
    void stop() override { playing = false; }
    void pause() override { paused = true; }
    void resume() override { playing = true; paused = false; }
    [[nodiscard]] bool isPlaying() override { return playing && !paused; }
    [[nodiscard]] bool isFinished() override { return false; }

    bool playing{false};
    bool paused{false};
};

class LoopbackRuntimeTransport final : public lofibox::runtime::RuntimeTransport {
public:
    explicit LoopbackRuntimeTransport(lofibox::runtime::RuntimeCommandServer& server) noexcept
        : server_(server)
    {
    }

    [[nodiscard]] lofibox::runtime::RuntimeCommandResponse sendCommand(const lofibox::runtime::RuntimeCommandRequest& request) override
    {
        return {server_.dispatch(request.command)};
    }

    [[nodiscard]] lofibox::runtime::RuntimeQueryResponse sendQuery(const lofibox::runtime::RuntimeQueryRequest& request) override
    {
        return {server_.query(request.query)};
    }

private:
    lofibox::runtime::RuntimeCommandServer& server_;
};

} // namespace

int main()
{
    auto services = lofibox::app::withNullRuntimeServices();
    services.playback.audio_backend = std::make_shared<RuntimeClientAudioBackend>();

    lofibox::app::AppControllerSet controllers{};
    controllers.bindServices(services);
    controllers.library.mutableModel().tracks.push_back(
        lofibox::app::TrackRecord{31, std::filesystem::path{"transport.mp3"}, "Transport", "Artist", "Album"});
    controllers.library.setSongsContextAll();

    lofibox::app::EqState eq{};
    lofibox::runtime::RuntimeSessionFacade session{
        lofibox::application::AppServiceRegistry{controllers, services},
        eq};
    lofibox::runtime::RuntimeCommandBus bus{session};
    lofibox::runtime::RuntimeCommandServer server{bus};
    lofibox::runtime::InProcessRuntimeCommandClient client{server};
    LoopbackRuntimeTransport transport{server};

    const auto result = client.dispatch(lofibox::runtime::RuntimeCommand{
        lofibox::runtime::RuntimeCommandKind::PlaybackStartTrack,
        lofibox::runtime::RuntimeCommandPayload::startTrack(31),
        lofibox::runtime::CommandOrigin::RuntimeCli,
        "runtime-cli-start"});
    if (!result.accepted
        || !result.applied
        || result.origin != lofibox::runtime::CommandOrigin::RuntimeCli
        || result.correlation_id != "runtime-cli-start"
        || result.version_before_apply != 0U
        || result.version_after_apply != 1U) {
        std::cerr << "Expected runtime client/server to preserve result envelope facts.\n";
        return 1;
    }

    const auto playback = client.query(lofibox::runtime::RuntimeQuery{
        lofibox::runtime::RuntimeQueryKind::PlaybackSnapshot,
        lofibox::runtime::CommandOrigin::RuntimeCli,
        "runtime-cli-now"});
    if (playback.playback.current_track_id != 31 || playback.playback.title != "Transport" || playback.version != 1U) {
        std::cerr << "Expected runtime client query to return the server snapshot.\n";
        return 1;
    }

    const auto unsupported = transport.sendCommand(lofibox::runtime::RuntimeCommandRequest{lofibox::runtime::RuntimeCommand{
        lofibox::runtime::RuntimeCommandKind::PlaybackSeek,
        lofibox::runtime::RuntimeCommandPayload::seek(42.0),
        lofibox::runtime::CommandOrigin::Automation,
        "seek-not-yet"}});
    if (unsupported.result.accepted
        || unsupported.result.applied
        || unsupported.result.origin != lofibox::runtime::CommandOrigin::Automation
        || unsupported.result.correlation_id != "seek-not-yet"
        || unsupported.result.version_before_apply != 1U
        || unsupported.result.version_after_apply != 1U
        || unsupported.result.code != "UNSUPPORTED_RUNTIME_COMMAND") {
        std::cerr << "Expected transport-neutral server envelope to reject unsupported commands without mutating version.\n";
        return 1;
    }

    const auto full = transport.sendQuery(lofibox::runtime::RuntimeQueryRequest{lofibox::runtime::RuntimeQuery{
        lofibox::runtime::RuntimeQueryKind::FullSnapshot,
        lofibox::runtime::CommandOrigin::Automation,
        "full"}});
    if (full.snapshot.version != 1U || full.snapshot.playback.current_track_id != 31) {
        std::cerr << "Expected transport-neutral query envelope to expose the same live runtime truth.\n";
        return 1;
    }

    return 0;
}
