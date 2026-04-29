// SPDX-License-Identifier: GPL-3.0-or-later

#include "cli/runtime_cli.h"

#include <cstdlib>
#include <filesystem>
#include <initializer_list>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "cli/cli_format.h"
#include "runtime/runtime_command.h"
#include "runtime/runtime_result.h"
#include "runtime/runtime_snapshot.h"
#include "runtime/unix_socket_runtime_transport.h"

namespace lofibox::cli {
namespace {

struct CliOption {
    std::string name{};
    std::string value{};
};

struct ParsedRuntimeArgs {
    std::vector<std::string_view> positional{};
    std::vector<CliOption> options{};
    std::optional<std::string> socket_path{};
};

bool isRuntimeCommand(std::string_view command) noexcept
{
    return command == "runtime"
        || command == "now"
        || command == "play"
        || command == "pause"
        || command == "resume"
        || command == "toggle"
        || command == "stop"
        || command == "seek"
        || command == "next"
        || command == "prev"
        || command == "queue"
        || command == "shuffle"
        || command == "repeat"
        || command == "eq"
        || command == "remote";
}

ParsedRuntimeArgs parseArgs(int argc, char** argv)
{
    ParsedRuntimeArgs args{};
    for (int index = 1; index < argc; ++index) {
        const std::string_view current{argv[index]};
        if (current == "--runtime-socket" && (index + 1) < argc) {
            args.socket_path = argv[++index];
            continue;
        }
        if (current.rfind("--", 0) == 0) {
            const auto eq = current.find('=');
            if (eq != std::string_view::npos) {
                args.options.push_back({std::string{current.substr(2, eq - 2)}, std::string{current.substr(eq + 1)}});
            } else if ((index + 1) < argc && std::string_view{argv[index + 1]}.rfind("-", 0) != 0) {
                args.options.push_back({std::string{current.substr(2)}, argv[index + 1]});
                ++index;
            } else {
                args.options.push_back({std::string{current.substr(2)}, "true"});
            }
            continue;
        }
        args.positional.push_back(current);
    }
    return args;
}

std::optional<std::string> optionValue(const ParsedRuntimeArgs& args, std::string_view name)
{
    for (const auto& option : args.options) {
        if (option.name == name) {
            return option.value;
        }
    }
    return std::nullopt;
}

bool optionPresent(const ParsedRuntimeArgs& args, std::string_view name)
{
    return optionValue(args, name).has_value();
}

CliOutputOptions outputOptions(const ParsedRuntimeArgs& args)
{
    CliOutputOptions options{};
    options.json = optionPresent(args, "json");
    options.porcelain = optionPresent(args, "porcelain");
    options.quiet = optionPresent(args, "quiet");
    if (const auto fields = optionValue(args, "fields")) {
        options.fields = splitFields(*fields);
    }
    return options;
}

std::string correlation(std::string_view action)
{
    return "cli:" + std::string{action};
}

lofibox::runtime::RuntimeCommand command(lofibox::runtime::RuntimeCommandKind kind, lofibox::runtime::RuntimeCommandPayload payload = {})
{
    return lofibox::runtime::RuntimeCommand{kind, std::move(payload), lofibox::runtime::CommandOrigin::RuntimeCli, correlation("runtime")};
}

std::optional<int> parseInt(std::string_view value)
{
    try {
        return std::stoi(std::string{value});
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<double> parseDouble(std::string_view value)
{
    try {
        return std::stod(std::string{value});
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<double> parseDuration(std::string_view value)
{
    if (value.empty()) {
        return std::nullopt;
    }
    bool relative = false;
    if (value.front() == '+') {
        relative = true;
        value.remove_prefix(1);
    }
    if (!value.empty() && value.back() == 's') {
        value.remove_suffix(1);
    }
    const auto colon = value.find(':');
    if (colon != std::string_view::npos) {
        const auto minutes = parseDouble(value.substr(0, colon));
        const auto seconds = parseDouble(value.substr(colon + 1));
        if (!minutes || !seconds) {
            return std::nullopt;
        }
        return (*minutes * 60.0) + *seconds;
    }
    (void)relative;
    return parseDouble(value);
}

void printResult(const lofibox::runtime::RuntimeCommandResult& result, const CliOutputOptions& options, std::ostream& out)
{
    if (options.quiet) {
        return;
    }
    if (options.json) {
        out << '{';
        bool first = true;
        printJsonBoolField(out, "accepted", result.accepted, first);
        printJsonBoolField(out, "applied", result.applied, first);
        printJsonField(out, "code", result.code, first);
        printJsonField(out, "message", result.message, first);
        printJsonField(out, "correlation_id", result.correlation_id, first);
        printJsonNumberField(out, "version_before_apply", std::to_string(result.version_before_apply), first);
        printJsonNumberField(out, "version_after_apply", std::to_string(result.version_after_apply), first);
        out << "}\n";
        return;
    }
    if (!options.porcelain) {
        out << "Code: " << result.code << '\n'
            << "Applied: " << (result.applied ? "yes" : "no") << '\n'
            << "Message: " << result.message << '\n';
        return;
    }
    out << result.code << '\t' << (result.applied ? "APPLIED" : "NOT_APPLIED") << '\t' << result.message << '\n';
}

std::string playbackStatusLabel(lofibox::runtime::RuntimePlaybackStatus status)
{
    switch (status) {
    case lofibox::runtime::RuntimePlaybackStatus::Empty: return "EMPTY";
    case lofibox::runtime::RuntimePlaybackStatus::Paused: return "PAUSED";
    case lofibox::runtime::RuntimePlaybackStatus::Playing: return "PLAYING";
    }
    return "UNKNOWN";
}

std::string idsLabel(const std::vector<int>& ids)
{
    std::ostringstream out{};
    for (std::size_t index = 0; index < ids.size(); ++index) {
        if (index > 0U) {
            out << ',';
        }
        out << ids[index];
    }
    return out.str();
}

CliFields playbackFields(const lofibox::runtime::PlaybackRuntimeSnapshot& playback)
{
    return {
        {"status", playbackStatusLabel(playback.status)},
        {"track", playback.current_track_id ? std::to_string(*playback.current_track_id) : "-"},
        {"title", playback.title.empty() ? "-" : playback.title},
        {"source", playback.source_label.empty() ? "-" : playback.source_label},
        {"elapsed", std::to_string(playback.elapsed_seconds)},
        {"duration", std::to_string(playback.duration_seconds)},
        {"audio", playback.audio_active ? "ACTIVE" : "IDLE"},
        {"shuffle", playback.shuffle_enabled ? "ON" : "OFF"},
        {"repeat", playback.repeat_one ? "ONE" : (playback.repeat_all ? "ALL" : "OFF")},
        {"version", std::to_string(playback.version)},
    };
}

CliFields queueFields(const lofibox::runtime::QueueRuntimeSnapshot& queue)
{
    return {
        {"index", std::to_string(queue.active_index)},
        {"tracks", idsLabel(queue.active_ids)},
        {"shuffle", queue.shuffle_enabled ? "ON" : "OFF"},
        {"repeat", queue.repeat_one ? "ONE" : (queue.repeat_all ? "ALL" : "OFF")},
        {"version", std::to_string(queue.version)},
    };
}

std::string bandsLabel(const lofibox::runtime::EqRuntimeSnapshot& eq)
{
    std::ostringstream out{};
    bool first = true;
    for (const int gain : eq.bands) {
        if (!first) out << ',';
        first = false;
        out << gain;
    }
    return out.str();
}

CliFields eqFields(const lofibox::runtime::EqRuntimeSnapshot& eq)
{
    return {
        {"enabled", eq.enabled ? "ON" : "OFF"},
        {"preset", eq.preset_name},
        {"bands", bandsLabel(eq)},
        {"version", std::to_string(eq.version)},
    };
}

CliFields remoteFields(const lofibox::runtime::RemoteSessionSnapshot& remote)
{
    return {
        {"profile", remote.profile_id.empty() ? "-" : remote.profile_id},
        {"source", remote.source_label},
        {"connection", remote.connection_status},
        {"stream", remote.stream_resolved ? "RESOLVED" : "EMPTY"},
        {"url", remote.redacted_url.empty() ? "-" : remote.redacted_url},
        {"buffer", remote.buffer_state},
        {"recovery", remote.recovery_action},
        {"bitrate", std::to_string(remote.bitrate_kbps)},
        {"codec", remote.codec.empty() ? "-" : remote.codec},
        {"live", remote.live ? "YES" : "NO"},
        {"seekable", remote.seekable ? "YES" : "NO"},
        {"version", std::to_string(remote.version)},
    };
}

CliFields settingsFields(const lofibox::runtime::SettingsRuntimeSnapshot& settings)
{
    return {
        {"output", settings.output_mode},
        {"network", settings.network_policy},
        {"sleep", settings.sleep_timer},
        {"version", std::to_string(settings.version)},
    };
}

void printPlayback(const lofibox::runtime::PlaybackRuntimeSnapshot& playback, const CliOutputOptions& options, std::ostream& out)
{
    printObject(out, options, playbackFields(playback));
}

void printQueue(const lofibox::runtime::QueueRuntimeSnapshot& queue, const CliOutputOptions& options, std::ostream& out)
{
    printObject(out, options, queueFields(queue));
}

void printEq(const lofibox::runtime::EqRuntimeSnapshot& eq, const CliOutputOptions& options, std::ostream& out)
{
    printObject(out, options, eqFields(eq));
}

void printRemote(const lofibox::runtime::RemoteSessionSnapshot& remote, const CliOutputOptions& options, std::ostream& out)
{
    printObject(out, options, remoteFields(remote));
}

void printSettings(const lofibox::runtime::SettingsRuntimeSnapshot& settings, const CliOutputOptions& options, std::ostream& out)
{
    printObject(out, options, settingsFields(settings));
}

void printFull(const lofibox::runtime::RuntimeSnapshot& snapshot, const CliOutputOptions& options, std::ostream& out)
{
    if (options.quiet) {
        return;
    }
    if (!options.json) {
        CliFields fields = {{"version", std::to_string(snapshot.version)}};
        const auto playback = playbackFields(snapshot.playback);
        fields.insert(fields.end(), playback.begin(), playback.end());
        printObject(out, options, fields);
        return;
    }
    out << "{\"version\":" << snapshot.version << ",\"playback\":{";
    bool first = true;
    for (const auto& [name, value] : playbackFields(snapshot.playback)) {
        if (wantsField(options, name)) printJsonField(out, name, value, first);
    }
    out << "},\"queue\":{";
    first = true;
    for (const auto& [name, value] : queueFields(snapshot.queue)) {
        printJsonField(out, name, value, first);
    }
    out << "},\"eq\":{";
    first = true;
    for (const auto& [name, value] : eqFields(snapshot.eq)) {
        printJsonField(out, name, value, first);
    }
    out << "},\"remote\":{";
    first = true;
    for (const auto& [name, value] : remoteFields(snapshot.remote)) {
        printJsonField(out, name, value, first);
    }
    out << "},\"settings\":{";
    first = true;
    for (const auto& [name, value] : settingsFields(snapshot.settings)) {
        printJsonField(out, name, value, first);
    }
    out << "}}\n";
}

std::optional<lofibox::runtime::RuntimeCommand> buildCommand(const ParsedRuntimeArgs& args, std::ostream& err)
{
    const auto& p = args.positional;
    if (p.empty()) {
        return std::nullopt;
    }
    const auto first = p[0];
    if (first == "play") {
        if (optionPresent(args, "pause")) return command(lofibox::runtime::RuntimeCommandKind::PlaybackPause);
        if (optionPresent(args, "resume")) return command(lofibox::runtime::RuntimeCommandKind::PlaybackResume);
        if (optionPresent(args, "toggle")) return command(lofibox::runtime::RuntimeCommandKind::PlaybackToggle);
        if (optionPresent(args, "next")) return command(lofibox::runtime::RuntimeCommandKind::QueueStep, lofibox::runtime::RuntimeCommandPayload::queueStep(1));
        if (optionPresent(args, "prev")) return command(lofibox::runtime::RuntimeCommandKind::QueueStep, lofibox::runtime::RuntimeCommandPayload::queueStep(-1));
        if (optionPresent(args, "stop")) return command(lofibox::runtime::RuntimeCommandKind::PlaybackStop);
        if (const auto id = optionValue(args, "id")) {
            const auto track_id = parseInt(*id);
            if (!track_id) {
                err << "play --id requires a numeric track id.\n";
                return std::nullopt;
            }
            return command(lofibox::runtime::RuntimeCommandKind::PlaybackStartTrack, lofibox::runtime::RuntimeCommandPayload::startTrack(*track_id));
        }
        if (const auto seek = optionValue(args, "seek")) {
            const auto seconds = parseDuration(*seek);
            if (!seconds) {
                err << "play --seek requires seconds or MM:SS.\n";
                return std::nullopt;
            }
            return command(lofibox::runtime::RuntimeCommandKind::PlaybackSeek, lofibox::runtime::RuntimeCommandPayload::seek(*seconds));
        }
        if (const auto shuffle = optionValue(args, "shuffle")) {
            if (*shuffle == "on" || *shuffle == "off") {
                return command(lofibox::runtime::RuntimeCommandKind::PlaybackToggleShuffle);
            }
        }
        if (const auto repeat = optionValue(args, "repeat")) {
            if (*repeat == "all") return command(lofibox::runtime::RuntimeCommandKind::PlaybackSetRepeatAll, lofibox::runtime::RuntimeCommandPayload::enabled(true));
            if (*repeat == "one") return command(lofibox::runtime::RuntimeCommandKind::PlaybackSetRepeatOne, lofibox::runtime::RuntimeCommandPayload::enabled(true));
            if (*repeat == "off") return command(lofibox::runtime::RuntimeCommandKind::PlaybackSetRepeatAll, lofibox::runtime::RuntimeCommandPayload::enabled(false));
        }
        return command(lofibox::runtime::RuntimeCommandKind::PlaybackPlay);
    }
    if (first == "pause") return command(lofibox::runtime::RuntimeCommandKind::PlaybackPause);
    if (first == "resume") return command(lofibox::runtime::RuntimeCommandKind::PlaybackResume);
    if (first == "toggle") return command(lofibox::runtime::RuntimeCommandKind::PlaybackToggle);
    if (first == "stop") return command(lofibox::runtime::RuntimeCommandKind::PlaybackStop);
    if (first == "next") return command(lofibox::runtime::RuntimeCommandKind::QueueStep, lofibox::runtime::RuntimeCommandPayload::queueStep(1));
    if (first == "prev") return command(lofibox::runtime::RuntimeCommandKind::QueueStep, lofibox::runtime::RuntimeCommandPayload::queueStep(-1));
    if (first == "seek" && p.size() >= 2) {
        const auto seconds = parseDuration(p[1]);
        if (!seconds) {
            err << "seek requires seconds or MM:SS.\n";
            return std::nullopt;
        }
        return command(lofibox::runtime::RuntimeCommandKind::PlaybackSeek, lofibox::runtime::RuntimeCommandPayload::seek(*seconds));
    }
    if (first == "queue" && p.size() >= 2) {
        if (p[1] == "next") {
            return command(lofibox::runtime::RuntimeCommandKind::QueueStep, lofibox::runtime::RuntimeCommandPayload::queueStep(1));
        }
        if (p[1] == "set" && p.size() >= 3) {
            const auto index = parseInt(p[2]);
            if (!index) {
                err << "queue set requires numeric index.\n";
                return std::nullopt;
            }
            return command(lofibox::runtime::RuntimeCommandKind::QueueJump, lofibox::runtime::RuntimeCommandPayload::queueIndex(*index));
        }
        if (p[1] == "jump" && p.size() >= 3) {
            const auto index = parseInt(p[2]);
            if (!index) {
                err << "queue jump requires numeric index.\n";
                return std::nullopt;
            }
            return command(lofibox::runtime::RuntimeCommandKind::QueueJump, lofibox::runtime::RuntimeCommandPayload::queueIndex(*index));
        }
        if (p[1] == "clear") {
            return command(lofibox::runtime::RuntimeCommandKind::QueueClear);
        }
    }
    if (first == "shuffle" && p.size() >= 2) {
        if (p[1] == "on") return command(lofibox::runtime::RuntimeCommandKind::PlaybackToggleShuffle);
        if (p[1] == "off") return command(lofibox::runtime::RuntimeCommandKind::PlaybackToggleShuffle);
    }
    if (first == "repeat" && p.size() >= 2) {
        if (p[1] == "all") return command(lofibox::runtime::RuntimeCommandKind::PlaybackSetRepeatAll, lofibox::runtime::RuntimeCommandPayload::enabled(true));
        if (p[1] == "one") return command(lofibox::runtime::RuntimeCommandKind::PlaybackSetRepeatOne, lofibox::runtime::RuntimeCommandPayload::enabled(true));
        if (p[1] == "off") return command(lofibox::runtime::RuntimeCommandKind::PlaybackSetRepeatAll, lofibox::runtime::RuntimeCommandPayload::enabled(false));
    }
    if (first == "eq" && p.size() >= 2) {
        if (p[1] == "enable") return command(lofibox::runtime::RuntimeCommandKind::EqEnable);
        if (p[1] == "disable") return command(lofibox::runtime::RuntimeCommandKind::EqDisable);
        if (p[1] == "reset") return command(lofibox::runtime::RuntimeCommandKind::EqReset);
        if (p[1] == "preset" && p.size() >= 3) return command(lofibox::runtime::RuntimeCommandKind::EqApplyPreset, lofibox::runtime::RuntimeCommandPayload::eqApplyPreset(std::string{p[2]}));
        if (p[1] == "band" && p.size() >= 4) {
            const auto band = parseInt(p[2]);
            const auto gain = parseInt(p[3]);
            if (!band || !gain) {
                err << "eq band requires numeric band and gain.\n";
                return std::nullopt;
            }
            return command(lofibox::runtime::RuntimeCommandKind::EqSetBand, lofibox::runtime::RuntimeCommandPayload::eqSetBand(*band, *gain));
        }
    }
    if (first == "remote" && p.size() >= 2 && p[1] == "reconnect") {
        return command(lofibox::runtime::RuntimeCommandKind::RemoteReconnect);
    }
    if (first == "runtime" && p.size() >= 2) {
        if (p[1] == "reload") return command(lofibox::runtime::RuntimeCommandKind::RuntimeReload);
        if (p[1] == "shutdown") return command(lofibox::runtime::RuntimeCommandKind::RuntimeShutdown);
        if (p[1] == "settings" && p.size() >= 3 && p[2] == "apply") {
            const auto output = p.size() >= 4 ? std::string{p[3]} : std::string{};
            const auto network = p.size() >= 5 ? std::string{p[4]} : std::string{};
            const auto sleep = p.size() >= 6 ? std::string{p[5]} : std::string{};
            return command(lofibox::runtime::RuntimeCommandKind::SettingsApplyLive, lofibox::runtime::RuntimeCommandPayload::settingsApplyLive(output, network, sleep));
        }
    }
    return std::nullopt;
}

std::optional<lofibox::runtime::RuntimeQuery> buildQuery(const ParsedRuntimeArgs& args)
{
    const auto& p = args.positional;
    if (p.empty()) {
        return std::nullopt;
    }
    if (p[0] == "now") {
        return lofibox::runtime::RuntimeQuery{lofibox::runtime::RuntimeQueryKind::FullSnapshot, lofibox::runtime::CommandOrigin::RuntimeCli, correlation("now")};
    }
    if (p[0] == "queue" && p.size() >= 2 && p[1] == "show") {
        return lofibox::runtime::RuntimeQuery{lofibox::runtime::RuntimeQueryKind::QueueSnapshot, lofibox::runtime::CommandOrigin::RuntimeCli, correlation("queue")};
    }
    if (p[0] == "eq" && p.size() >= 2 && p[1] == "show") {
        return lofibox::runtime::RuntimeQuery{lofibox::runtime::RuntimeQueryKind::EqSnapshot, lofibox::runtime::CommandOrigin::RuntimeCli, correlation("eq")};
    }
    if (p[0] != "runtime") {
        return std::nullopt;
    }
    if (p.size() == 1 || p[1] == "status") {
        return lofibox::runtime::RuntimeQuery{lofibox::runtime::RuntimeQueryKind::FullSnapshot, lofibox::runtime::CommandOrigin::RuntimeCli, correlation("status")};
    }
    if (p[1] == "playback") {
        return lofibox::runtime::RuntimeQuery{lofibox::runtime::RuntimeQueryKind::PlaybackSnapshot, lofibox::runtime::CommandOrigin::RuntimeCli, correlation("playback")};
    }
    if (p[1] == "queue") {
        return lofibox::runtime::RuntimeQuery{lofibox::runtime::RuntimeQueryKind::QueueSnapshot, lofibox::runtime::CommandOrigin::RuntimeCli, correlation("queue")};
    }
    if (p[1] == "eq") {
        return lofibox::runtime::RuntimeQuery{lofibox::runtime::RuntimeQueryKind::EqSnapshot, lofibox::runtime::CommandOrigin::RuntimeCli, correlation("eq")};
    }
    if (p[1] == "remote") {
        return lofibox::runtime::RuntimeQuery{lofibox::runtime::RuntimeQueryKind::RemoteSessionSnapshot, lofibox::runtime::CommandOrigin::RuntimeCli, correlation("remote")};
    }
    if (p[1] == "settings") {
        return lofibox::runtime::RuntimeQuery{lofibox::runtime::RuntimeQueryKind::SettingsSnapshot, lofibox::runtime::CommandOrigin::RuntimeCli, correlation("settings")};
    }
    return std::nullopt;
}

} // namespace

std::optional<int> runRuntimeCliCommand(int argc, char** argv, std::ostream& out, std::ostream& err)
{
    const auto args = parseArgs(argc, argv);
    if (args.positional.empty() || !isRuntimeCommand(args.positional.front())) {
        return std::nullopt;
    }
    if (args.positional.front() == "remote"
        && (args.positional.size() < 2U || args.positional[1] != "reconnect")) {
        return std::nullopt;
    }
    const auto format = outputOptions(args);

    lofibox::runtime::UnixSocketRuntimeCommandClient client{
        args.socket_path ? std::filesystem::path{*args.socket_path} : lofibox::runtime::defaultRuntimeSocketPath()};

    if (const auto query = buildQuery(args)) {
        const auto snapshot = client.query(*query);
        if (!client.lastError().empty()) {
            err << client.lastError() << '\n';
            return static_cast<int>(CliExitCode::Network);
        }
        switch (query->kind) {
        case lofibox::runtime::RuntimeQueryKind::PlaybackSnapshot: printPlayback(snapshot.playback, format, out); break;
        case lofibox::runtime::RuntimeQueryKind::QueueSnapshot: printQueue(snapshot.queue, format, out); break;
        case lofibox::runtime::RuntimeQueryKind::EqSnapshot: printEq(snapshot.eq, format, out); break;
        case lofibox::runtime::RuntimeQueryKind::RemoteSessionSnapshot: printRemote(snapshot.remote, format, out); break;
        case lofibox::runtime::RuntimeQueryKind::SettingsSnapshot: printSettings(snapshot.settings, format, out); break;
        case lofibox::runtime::RuntimeQueryKind::FullSnapshot: printFull(snapshot, format, out); break;
        }
        return 0;
    }

    const auto command_value = buildCommand(args, err);
    if (!command_value) {
        err << "Unknown runtime command.\n";
        return static_cast<int>(CliExitCode::Usage);
    }
    const auto result = client.dispatch(*command_value);
    if (!client.lastError().empty()) {
        err << client.lastError() << '\n';
        return static_cast<int>(CliExitCode::Network);
    }
    printResult(result, format, out);
    if (!result.accepted) {
        return static_cast<int>(CliExitCode::Usage);
    }
    if (!result.applied && command_value->kind == lofibox::runtime::RuntimeCommandKind::PlaybackPlay) {
        return static_cast<int>(CliExitCode::Playback);
    }
    return 0;
}

} // namespace lofibox::cli
