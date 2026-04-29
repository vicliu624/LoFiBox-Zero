// SPDX-License-Identifier: GPL-3.0-or-later

#include "cli/runtime_cli.h"

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "runtime/runtime_command.h"
#include "runtime/runtime_result.h"
#include "runtime/runtime_snapshot.h"
#include "runtime/unix_socket_runtime_transport.h"

namespace lofibox::cli {
namespace {

struct ParsedRuntimeArgs {
    std::vector<std::string_view> positional{};
    std::optional<std::string> socket_path{};
};

bool isRuntimeCommand(std::string_view command) noexcept
{
    return command == "runtime"
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
        args.positional.push_back(current);
    }
    return args;
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

void printResult(const lofibox::runtime::RuntimeCommandResult& result, std::ostream& out)
{
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

void printPlayback(const lofibox::runtime::PlaybackRuntimeSnapshot& playback, std::ostream& out)
{
    out << "STATUS\t" << playbackStatusLabel(playback.status) << '\n'
        << "TRACK\t" << (playback.current_track_id ? std::to_string(*playback.current_track_id) : std::string{"-"}) << '\n'
        << "TITLE\t" << (playback.title.empty() ? "-" : playback.title) << '\n'
        << "SOURCE\t" << (playback.source_label.empty() ? "-" : playback.source_label) << '\n'
        << "ELAPSED\t" << playback.elapsed_seconds << '\n'
        << "DURATION\t" << playback.duration_seconds << '\n'
        << "AUDIO\t" << (playback.audio_active ? "ACTIVE" : "IDLE") << '\n'
        << "SHUFFLE\t" << (playback.shuffle_enabled ? "ON" : "OFF") << '\n'
        << "REPEAT\t" << (playback.repeat_one ? "ONE" : (playback.repeat_all ? "ALL" : "OFF")) << '\n';
}

void printQueue(const lofibox::runtime::QueueRuntimeSnapshot& queue, std::ostream& out)
{
    out << "INDEX\t" << queue.active_index << '\n';
    out << "TRACKS";
    for (const int id : queue.active_ids) {
        out << '\t' << id;
    }
    out << '\n';
}

void printEq(const lofibox::runtime::EqRuntimeSnapshot& eq, std::ostream& out)
{
    out << "ENABLED\t" << (eq.enabled ? "ON" : "OFF") << '\n'
        << "PRESET\t" << eq.preset_name << '\n'
        << "BANDS";
    for (const int gain : eq.bands) {
        out << '\t' << gain;
    }
    out << '\n';
}

void printRemote(const lofibox::runtime::RemoteSessionSnapshot& remote, std::ostream& out)
{
    out << "PROFILE\t" << (remote.profile_id.empty() ? "-" : remote.profile_id) << '\n'
        << "SOURCE\t" << remote.source_label << '\n'
        << "CONNECTION\t" << remote.connection_status << '\n'
        << "STREAM\t" << (remote.stream_resolved ? "RESOLVED" : "EMPTY") << '\n'
        << "URL\t" << (remote.redacted_url.empty() ? "-" : remote.redacted_url) << '\n'
        << "BUFFER\t" << remote.buffer_state << '\n'
        << "RECOVERY\t" << remote.recovery_action << '\n'
        << "BITRATE\t" << remote.bitrate_kbps << '\n'
        << "CODEC\t" << (remote.codec.empty() ? "-" : remote.codec) << '\n'
        << "LIVE\t" << (remote.live ? "YES" : "NO") << '\n'
        << "SEEKABLE\t" << (remote.seekable ? "YES" : "NO") << '\n';
}

void printSettings(const lofibox::runtime::SettingsRuntimeSnapshot& settings, std::ostream& out)
{
    out << "OUTPUT\t" << settings.output_mode << '\n'
        << "NETWORK\t" << settings.network_policy << '\n'
        << "SLEEP\t" << settings.sleep_timer << '\n';
}

void printFull(const lofibox::runtime::RuntimeSnapshot& snapshot, std::ostream& out)
{
    out << "VERSION\t" << snapshot.version << '\n';
    printPlayback(snapshot.playback, out);
}

std::optional<lofibox::runtime::RuntimeCommand> buildCommand(const ParsedRuntimeArgs& args, std::ostream& err)
{
    const auto& p = args.positional;
    if (p.empty()) {
        return std::nullopt;
    }
    const auto first = p[0];
    if (first == "play") return command(lofibox::runtime::RuntimeCommandKind::PlaybackPlay);
    if (first == "pause") return command(lofibox::runtime::RuntimeCommandKind::PlaybackPause);
    if (first == "resume") return command(lofibox::runtime::RuntimeCommandKind::PlaybackResume);
    if (first == "toggle") return command(lofibox::runtime::RuntimeCommandKind::PlaybackToggle);
    if (first == "stop") return command(lofibox::runtime::RuntimeCommandKind::PlaybackStop);
    if (first == "next") return command(lofibox::runtime::RuntimeCommandKind::QueueStep, lofibox::runtime::RuntimeCommandPayload::queueStep(1));
    if (first == "prev") return command(lofibox::runtime::RuntimeCommandKind::QueueStep, lofibox::runtime::RuntimeCommandPayload::queueStep(-1));
    if (first == "seek" && p.size() >= 2) {
        const auto seconds = parseDouble(p[1]);
        if (!seconds) {
            err << "seek requires numeric seconds.\n";
            return std::nullopt;
        }
        return command(lofibox::runtime::RuntimeCommandKind::PlaybackSeek, lofibox::runtime::RuntimeCommandPayload::seek(*seconds));
    }
    if (first == "queue" && p.size() >= 2) {
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
    if (p.empty() || p[0] != "runtime") {
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

    lofibox::runtime::UnixSocketRuntimeCommandClient client{
        args.socket_path ? std::filesystem::path{*args.socket_path} : lofibox::runtime::defaultRuntimeSocketPath()};

    if (const auto query = buildQuery(args)) {
        const auto snapshot = client.query(*query);
        if (!client.lastError().empty()) {
            err << client.lastError() << '\n';
            return 2;
        }
        switch (query->kind) {
        case lofibox::runtime::RuntimeQueryKind::PlaybackSnapshot: printPlayback(snapshot.playback, out); break;
        case lofibox::runtime::RuntimeQueryKind::QueueSnapshot: printQueue(snapshot.queue, out); break;
        case lofibox::runtime::RuntimeQueryKind::EqSnapshot: printEq(snapshot.eq, out); break;
        case lofibox::runtime::RuntimeQueryKind::RemoteSessionSnapshot: printRemote(snapshot.remote, out); break;
        case lofibox::runtime::RuntimeQueryKind::SettingsSnapshot: printSettings(snapshot.settings, out); break;
        case lofibox::runtime::RuntimeQueryKind::FullSnapshot: printFull(snapshot, out); break;
        }
        return 0;
    }

    const auto command_value = buildCommand(args, err);
    if (!command_value) {
        err << "Unknown runtime command.\n";
        return 64;
    }
    const auto result = client.dispatch(*command_value);
    if (!client.lastError().empty()) {
        err << client.lastError() << '\n';
        return 2;
    }
    printResult(result, out);
    return result.accepted ? 0 : 1;
}

} // namespace lofibox::cli
