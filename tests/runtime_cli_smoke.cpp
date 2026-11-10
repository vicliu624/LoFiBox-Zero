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
        ++starts;
        playing = true;
        return true;
    }
    void stop() override { playing = false; }
    [[nodiscard]] bool isPlaying() override { return playing; }
    [[nodiscard]] bool isFinished() override { return false; }

    bool playing{false};
    int starts{0};
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
    auto audio_backend = std::make_shared<RuntimeCliAudioBackend>();
    services.playback.audio_backend = audio_backend;

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

    for (const auto invalid : {"nan", "inf", "-inf", "12junk", "1e308:1"}) {
        out.str({}); err.str({});
        const auto rejected = runCli({"lofibox", "--runtime-socket", socket_path.string(), "seek", invalid, "--json"}, out, err);
        if (!rejected || *rejected != 2) {
            std::cerr << "Expected malformed/non-finite duration to be rejected: " << invalid << '\n';
            return 1;
        }
    }
    out.str({}); err.str({});
    const auto bad_index = runCli({"lofibox", "--runtime-socket", socket_path.string(), "play", "--id", "61junk", "--json"}, out, err);
    if (!bad_index || *bad_index != 2) return 1;

    out.str({}); err.str({});
    if (runCli({"lofibox", "--runtime-socket", socket_path.string(), "repeat", "one"}, out, err) != 0) return 1;
    if (runCli({"lofibox", "--runtime-socket", socket_path.string(), "repeat", "off"}, out, err) != 0) return 1;
    out.str({}); err.str({});
    if (runCli({"lofibox", "--runtime-socket", socket_path.string(), "now", "--fields", "repeat", "--json"}, out, err) != 0
        || out.str().find("\"repeat\":\"OFF\"") == std::string::npos) return 1;

    out.str({}); err.str({});
    if (runCli({"lofibox", "--runtime-socket", socket_path.string(), "play", "--id", "61"}, out, err) != 0) return 1;
    const int starts_before_rejection = audio_backend->starts;
    for (const auto& rejected_args : std::vector<std::vector<std::string>>{
             {"play", "--id", "99999"}, {"seek", "1e308"}, {"eq", "band", "999", "1"}}) {
        std::vector<std::string> args{"lofibox", "--runtime-socket", socket_path.string(), "--json"};
        args.insert(args.end(), rejected_args.begin(), rejected_args.end());
        out.str({}); err.str({});
        const auto rejected = runCli(std::move(args), out, err);
        const int expected = rejected_args.front() == "eq" ? 2 : 6;
        if (!rejected || *rejected != expected || out.str().find("\"applied\":false") == std::string::npos
            || audio_backend->starts != starts_before_rejection) {
            std::cerr << "Expected an unapplied runtime command to return a failure exit code: " << out.str() << err.str() << '\n';
            return 1;
        }
    }

    out.str({}); err.str({});
    exit_code = runCli({"lofibox", "--runtime-socket", socket_path.string(), "runtime", "settings", "apply", "ALSA", "LAN_ONLY", "15M", "--json"}, out, err);
    if (!exit_code || *exit_code != 0 || out.str().find("\"code\":\"SETTINGS_APPLY_LIVE\"") == std::string::npos) {
        std::cerr << "Expected settings apply to dispatch a command rather than query: " << out.str() << err.str() << '\n';
        return 1;
    }
    out.str({}); err.str({});
    exit_code = runCli({"lofibox", "--runtime-socket", socket_path.string(), "runtime", "settings", "--json"}, out, err);
    if (!exit_code || *exit_code != 0 || out.str().find("15M") == std::string::npos
        || out.str().find("LAN_ONLY") == std::string::npos || out.str().find("ALSA") == std::string::npos) {
        std::cerr << "Expected settings query to return the applied values: " << out.str() << err.str() << '\n';
        return 1;
    }

    runtime_host.stopExternalTransport();
    std::filesystem::remove(socket_path, ec);
#endif
    return 0;
}
