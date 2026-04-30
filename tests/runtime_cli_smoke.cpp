// SPDX-License-Identifier: GPL-3.0-or-later

#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "app/runtime_services.h"
#include "application/app_service_host.h"
#include "cli/runtime_cli.h"
#include "runtime/runtime_host.h"

namespace {

class RuntimeCliAudioBackend final : public lofibox::app::AudioPlaybackBackend {
public:
    [[nodiscard]] bool available() const override { return true; }
    [[nodiscard]] std::string displayName() const override { return "RUNTIME-CLI-TEST"; }
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

std::optional<int> runCli(std::vector<std::string> args, std::ostream& out, std::ostream& err)
{
    std::vector<char*> argv{};
    argv.reserve(args.size());
    for (auto& arg : args) {
        argv.push_back(arg.data());
    }
    return lofibox::cli::runRuntimeCliCommand(static_cast<int>(argv.size()), argv.data(), out, err);
}

} // namespace

int main()
{
#if defined(__unix__) || defined(__APPLE__)
    auto services = lofibox::app::withNullRuntimeServices();
    services.playback.audio_backend = std::make_shared<RuntimeCliAudioBackend>();

    lofibox::application::AppServiceHost app_host{services};
    app_host.controllers().library.mutableModel().tracks.push_back(
        lofibox::app::TrackRecord{61, std::filesystem::path{"cli.mp3"}, "CLI", "Artist", "Album"});
    app_host.controllers().library.setSongsContextAll();

    const auto socket_path = std::filesystem::temp_directory_path() / "lofibox-runtime-cli-smoke.sock";
    std::error_code ec{};
    std::filesystem::remove(socket_path, ec);

    lofibox::runtime::RuntimeHost runtime_host{app_host.registry()};
    if (!runtime_host.startExternalTransport(socket_path)) {
        std::cerr << "Expected RuntimeHost to expose socket transport for runtime CLI.\n";
        return 1;
    }

    std::ostringstream out{};
    std::ostringstream err{};
    auto exit_code = runCli({"lofibox", "--runtime-socket", socket_path.string(), "eq", "band", "0", "7", "--json"}, out, err);
    if (!exit_code || *exit_code != 0 || out.str().find("\"code\":\"EQ_SET_BAND\"") == std::string::npos) {
        std::cerr << "Expected runtime CLI to send EQ command through external transport only: " << err.str() << '\n';
        return 1;
    }

    out.str({});
    out.clear();
    err.str({});
    err.clear();
    exit_code = runCli({"lofibox", "--runtime-socket", socket_path.string(), "eq", "show", "--fields", "bands", "--porcelain"}, out, err);
    if (!exit_code || *exit_code != 0 || out.str().find("bands\t7") == std::string::npos) {
        std::cerr << "Expected runtime CLI query to read EQ snapshot through external transport: " << out.str() << err.str() << '\n';
        return 1;
    }

    out.str({});
    out.clear();
    err.str({});
    err.clear();
    exit_code = runCli({"lofibox", "--runtime-socket", socket_path.string(), "now", "--json"}, out, err);
    if (!exit_code || *exit_code != 0 || out.str().find("\"playback\"") == std::string::npos) {
        std::cerr << "Expected now alias to query full runtime snapshot through external transport: " << out.str() << err.str() << '\n';
        return 1;
    }

    out.str({});
    out.clear();
    err.str({});
    err.clear();
    exit_code = runCli({"lofibox", "--runtime-socket", socket_path.string(), "now", "--fields", "playback.status,queue.index", "--json"}, out, err);
    if (!exit_code || *exit_code != 0
        || out.str().find("\"playback\"") == std::string::npos
        || out.str().find("\"queue\"") == std::string::npos
        || out.str().find("\"eq\"") != std::string::npos) {
        std::cerr << "Expected full snapshot nested fields to include only requested domains: " << out.str() << err.str() << '\n';
        return 1;
    }

    out.str({});
    out.clear();
    err.str({});
    err.clear();
    exit_code = runCli({"lofibox", "--runtime-socket", socket_path.string(), "now", "--fields", "status,title", "--json"}, out, err);
    if (!exit_code || *exit_code != 0
        || out.str().find("\"playback\"") == std::string::npos
        || out.str().find("\"queue\"") != std::string::npos) {
        std::cerr << "Expected bare full-snapshot fields to remain playback-scoped: " << out.str() << err.str() << '\n';
        return 1;
    }

    out.str({});
    out.clear();
    err.str({});
    err.clear();
    exit_code = runCli({"lofibox", "--runtime-socket", socket_path.string(), "runtime", "playback", "--fields", "does_not_exist", "--json"}, out, err);
    if (!exit_code || *exit_code != 2
        || err.str().find("\"code\":\"INVALID_FIELD\"") == std::string::npos
        || err.str().find("\"allowed\"") == std::string::npos) {
        std::cerr << "Expected unknown playback fields to return structured exit 2: " << out.str() << err.str() << '\n';
        return 1;
    }

    out.str({});
    out.clear();
    err.str({});
    err.clear();
    exit_code = runCli({"lofibox", "--runtime-socket", socket_path.string(), "play", "--id", "nope", "--json"}, out, err);
    if (!exit_code || *exit_code != 2
        || err.str().find("\"code\":\"INVALID_ARGUMENT\"") == std::string::npos
        || err.str().find("Unknown runtime command") != std::string::npos) {
        std::cerr << "Expected invalid play id to produce one structured error only: " << out.str() << err.str() << '\n';
        return 1;
    }

    out.str({});
    out.clear();
    err.str({});
    err.clear();
    exit_code = runCli({"lofibox", "--runtime-socket", socket_path.string(), "shuffle", "on", "--json"}, out, err);
    if (!exit_code || *exit_code != 0) {
        std::cerr << "Expected shuffle on to dispatch: " << out.str() << err.str() << '\n';
        return 1;
    }
    out.str({});
    out.clear();
    err.str({});
    err.clear();
    exit_code = runCli({"lofibox", "--runtime-socket", socket_path.string(), "shuffle", "on", "--json"}, out, err);
    if (!exit_code || *exit_code != 0) {
        std::cerr << "Expected repeated shuffle on to remain idempotent: " << out.str() << err.str() << '\n';
        return 1;
    }
    out.str({});
    out.clear();
    err.str({});
    err.clear();
    exit_code = runCli({"lofibox", "--runtime-socket", socket_path.string(), "runtime", "playback", "--fields", "shuffle", "--json"}, out, err);
    if (!exit_code || *exit_code != 0 || out.str().find("\"shuffle\":\"ON\"") == std::string::npos) {
        std::cerr << "Expected repeated shuffle on to leave shuffle ON: " << out.str() << err.str() << '\n';
        return 1;
    }
    out.str({});
    out.clear();
    err.str({});
    err.clear();
    exit_code = runCli({"lofibox", "--runtime-socket", socket_path.string(), "play", "--shuffle", "off", "--json"}, out, err);
    if (!exit_code || *exit_code != 0) {
        std::cerr << "Expected play --shuffle off to dispatch: " << out.str() << err.str() << '\n';
        return 1;
    }
    out.str({});
    out.clear();
    err.str({});
    err.clear();
    exit_code = runCli({"lofibox", "--runtime-socket", socket_path.string(), "runtime", "playback", "--fields", "shuffle", "--json"}, out, err);
    if (!exit_code || *exit_code != 0 || out.str().find("\"shuffle\":\"OFF\"") == std::string::npos) {
        std::cerr << "Expected play --shuffle off to set shuffle OFF: " << out.str() << err.str() << '\n';
        return 1;
    }

    runtime_host.stopExternalTransport();
    std::filesystem::remove(socket_path, ec);
#endif
    return 0;
}
