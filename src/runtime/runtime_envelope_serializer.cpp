// SPDX-License-Identifier: GPL-3.0-or-later

#include "runtime/runtime_envelope_serializer.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <vector>

namespace lofibox::runtime {
namespace {

template <typename Enum>
struct EnumName {
    Enum value;
    std::string_view name;
};

constexpr std::array<EnumName<RuntimeCommandKind>, 27> kCommandNames{{
    {RuntimeCommandKind::PlaybackPlay, "PlaybackPlay"},
    {RuntimeCommandKind::PlaybackPause, "PlaybackPause"},
    {RuntimeCommandKind::PlaybackResume, "PlaybackResume"},
    {RuntimeCommandKind::PlaybackToggle, "PlaybackToggle"},
    {RuntimeCommandKind::PlaybackStartTrack, "PlaybackStartTrack"},
    {RuntimeCommandKind::PlaybackStop, "PlaybackStop"},
    {RuntimeCommandKind::PlaybackSeek, "PlaybackSeek"},
    {RuntimeCommandKind::QueueStep, "QueueStep"},
    {RuntimeCommandKind::QueueJump, "QueueJump"},
    {RuntimeCommandKind::QueueClear, "QueueClear"},
    {RuntimeCommandKind::PlaybackToggleShuffle, "PlaybackToggleShuffle"},
    {RuntimeCommandKind::PlaybackCycleRepeat, "PlaybackCycleRepeat"},
    {RuntimeCommandKind::PlaybackCycleMainMenuMode, "PlaybackCycleMainMenuMode"},
    {RuntimeCommandKind::PlaybackSetRepeatAll, "PlaybackSetRepeatAll"},
    {RuntimeCommandKind::PlaybackSetRepeatOne, "PlaybackSetRepeatOne"},
    {RuntimeCommandKind::RemoteStartActiveStream, "RemoteStartActiveStream"},
    {RuntimeCommandKind::EqEnable, "EqEnable"},
    {RuntimeCommandKind::EqDisable, "EqDisable"},
    {RuntimeCommandKind::EqSetBand, "EqSetBand"},
    {RuntimeCommandKind::EqAdjustBand, "EqAdjustBand"},
    {RuntimeCommandKind::EqApplyPreset, "EqApplyPreset"},
    {RuntimeCommandKind::EqCyclePreset, "EqCyclePreset"},
    {RuntimeCommandKind::EqReset, "EqReset"},
    {RuntimeCommandKind::RemoteReconnect, "RemoteReconnect"},
    {RuntimeCommandKind::SettingsApplyLive, "SettingsApplyLive"},
    {RuntimeCommandKind::RuntimeShutdown, "RuntimeShutdown"},
    {RuntimeCommandKind::RuntimeReload, "RuntimeReload"},
}};

constexpr std::array<EnumName<RuntimeQueryKind>, 6> kQueryNames{{
    {RuntimeQueryKind::PlaybackSnapshot, "PlaybackSnapshot"},
    {RuntimeQueryKind::QueueSnapshot, "QueueSnapshot"},
    {RuntimeQueryKind::EqSnapshot, "EqSnapshot"},
    {RuntimeQueryKind::RemoteSessionSnapshot, "RemoteSessionSnapshot"},
    {RuntimeQueryKind::SettingsSnapshot, "SettingsSnapshot"},
    {RuntimeQueryKind::FullSnapshot, "FullSnapshot"},
}};

constexpr std::array<EnumName<CommandOrigin>, 6> kOriginNames{{
    {CommandOrigin::Gui, "Gui"},
    {CommandOrigin::DirectTest, "DirectTest"},
    {CommandOrigin::Desktop, "Desktop"},
    {CommandOrigin::RuntimeCli, "RuntimeCli"},
    {CommandOrigin::Automation, "Automation"},
    {CommandOrigin::Unknown, "Unknown"},
}};

template <typename Enum, std::size_t Size>
std::string_view enumName(Enum value, const std::array<EnumName<Enum>, Size>& names) noexcept
{
    for (const auto& entry : names) {
        if (entry.value == value) {
            return entry.name;
        }
    }
    return "Unknown";
}

template <typename Enum, std::size_t Size>
std::optional<Enum> enumFromName(std::string_view name, const std::array<EnumName<Enum>, Size>& names) noexcept
{
    for (const auto& entry : names) {
        if (entry.name == name) {
            return entry.value;
        }
    }
    return std::nullopt;
}

std::string escapeJson(std::string_view text)
{
    std::string out;
    out.reserve(text.size() + 2);
    for (const char ch : text) {
        switch (ch) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out += ch; break;
        }
    }
    return out;
}

void appendStringField(std::ostringstream& out, std::string_view key, std::string_view value)
{
    out << ",\"" << key << "\":\"" << escapeJson(value) << '"';
}

void appendBoolField(std::ostringstream& out, std::string_view key, bool value)
{
    out << ",\"" << key << "\":" << (value ? "true" : "false");
}

void appendIntArray(std::ostringstream& out, std::string_view key, const std::vector<int>& values)
{
    out << ",\"" << key << "\":[";
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index > 0) {
            out << ',';
        }
        out << values[index];
    }
    out << ']';
}

template <std::size_t Size>
void appendIntArray(std::ostringstream& out, std::string_view key, const std::array<int, Size>& values)
{
    out << ",\"" << key << "\":[";
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index > 0) {
            out << ',';
        }
        out << values[index];
    }
    out << ']';
}

std::optional<std::size_t> fieldValueStart(std::string_view json, std::string_view key)
{
    const auto quoted_key = "\"" + std::string{key} + "\":";
    const auto found = json.find(quoted_key);
    if (found == std::string_view::npos) {
        return std::nullopt;
    }
    auto pos = found + quoted_key.size();
    while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) {
        ++pos;
    }
    return pos;
}

std::optional<std::string> stringField(std::string_view json, std::string_view key)
{
    const auto start = fieldValueStart(json, key);
    if (!start || *start >= json.size() || json[*start] != '"') {
        return std::nullopt;
    }
    std::string value;
    for (std::size_t pos = *start + 1; pos < json.size(); ++pos) {
        const char ch = json[pos];
        if (ch == '"') {
            return value;
        }
        if (ch == '\\' && (pos + 1) < json.size()) {
            const char escaped = json[++pos];
            switch (escaped) {
            case 'n': value.push_back('\n'); break;
            case 'r': value.push_back('\r'); break;
            case 't': value.push_back('\t'); break;
            default: value.push_back(escaped); break;
            }
            continue;
        }
        value.push_back(ch);
    }
    return std::nullopt;
}

template <typename T>
std::optional<T> numberField(std::string_view json, std::string_view key)
{
    const auto start = fieldValueStart(json, key);
    if (!start) {
        return std::nullopt;
    }
    std::size_t end = *start;
    while (end < json.size() && (std::isdigit(static_cast<unsigned char>(json[end])) || json[end] == '-' || json[end] == '+' || json[end] == '.')) {
        ++end;
    }
    T value{};
    const auto text = json.substr(*start, end - *start);
    const auto* begin = text.data();
    const auto* finish = text.data() + text.size();
    const auto parsed = std::from_chars(begin, finish, value);
    if (parsed.ec != std::errc{}) {
        return std::nullopt;
    }
    return value;
}

std::optional<bool> boolField(std::string_view json, std::string_view key)
{
    const auto start = fieldValueStart(json, key);
    if (!start) {
        return std::nullopt;
    }
    if (json.substr(*start, 4) == "true") {
        return true;
    }
    if (json.substr(*start, 5) == "false") {
        return false;
    }
    return std::nullopt;
}

std::vector<int> intArrayField(std::string_view json, std::string_view key)
{
    std::vector<int> values;
    const auto start = fieldValueStart(json, key);
    if (!start || *start >= json.size() || json[*start] != '[') {
        return values;
    }
    const auto end = json.find(']', *start + 1);
    if (end == std::string_view::npos) {
        return values;
    }
    std::size_t pos = *start + 1;
    while (pos < end) {
        while (pos < end && (json[pos] == ',' || std::isspace(static_cast<unsigned char>(json[pos])))) {
            ++pos;
        }
        std::size_t next = pos;
        while (next < end && json[next] != ',') {
            ++next;
        }
        int value{};
        const auto text = json.substr(pos, next - pos);
        const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
        if (parsed.ec == std::errc{}) {
            values.push_back(value);
        }
        pos = next + 1;
    }
    return values;
}

void appendPayload(std::ostringstream& out, const RuntimeCommandPayload& payload)
{
    std::visit([&out](const auto& data) {
        using Payload = std::decay_t<decltype(data)>;
        if constexpr (std::is_same_v<Payload, EmptyPayload>) {
            appendStringField(out, "payload", "Empty");
        } else if constexpr (std::is_same_v<Payload, PlaybackStartTrackPayload>) {
            appendStringField(out, "payload", "PlaybackStartTrack");
            out << ",\"track_id\":" << data.track_id;
        } else if constexpr (std::is_same_v<Payload, PlaybackSeekPayload>) {
            appendStringField(out, "payload", "PlaybackSeek");
            out << ",\"seconds\":" << data.seconds;
        } else if constexpr (std::is_same_v<Payload, QueueStepPayload>) {
            appendStringField(out, "payload", "QueueStep");
            out << ",\"delta\":" << data.delta;
        } else if constexpr (std::is_same_v<Payload, QueueIndexPayload>) {
            appendStringField(out, "payload", "QueueIndex");
            out << ",\"queue_index\":" << data.queue_index;
        } else if constexpr (std::is_same_v<Payload, RuntimeEnabledPayload>) {
            appendStringField(out, "payload", "RuntimeEnabled");
            appendBoolField(out, "enabled", data.enabled);
        } else if constexpr (std::is_same_v<Payload, EqSetBandPayload>) {
            appendStringField(out, "payload", "EqSetBand");
            out << ",\"eq_band_index\":" << data.eq_band_index << ",\"eq_gain_db\":" << data.eq_gain_db;
        } else if constexpr (std::is_same_v<Payload, EqAdjustBandPayload>) {
            appendStringField(out, "payload", "EqAdjustBand");
            out << ",\"eq_band_index\":" << data.eq_band_index << ",\"eq_gain_delta\":" << data.eq_gain_delta;
        } else if constexpr (std::is_same_v<Payload, EqCyclePresetPayload>) {
            appendStringField(out, "payload", "EqCyclePreset");
            out << ",\"preset_delta\":" << data.preset_delta;
        } else if constexpr (std::is_same_v<Payload, EqApplyPresetPayload>) {
            appendStringField(out, "payload", "EqApplyPreset");
            appendStringField(out, "preset_name", data.preset_name);
        } else if constexpr (std::is_same_v<Payload, SettingsApplyLivePayload>) {
            appendStringField(out, "payload", "SettingsApplyLive");
            appendStringField(out, "output_mode", data.output_mode);
            appendStringField(out, "network_policy", data.network_policy);
            appendStringField(out, "sleep_timer", data.sleep_timer);
        } else if constexpr (std::is_same_v<Payload, RemotePlayResolvedStreamPayload>) {
            appendStringField(out, "payload", "RemotePlayResolvedStream");
            appendStringField(out, "source", data.source);
        } else if constexpr (std::is_same_v<Payload, RemotePlayResolvedLibraryTrackPayload>) {
            appendStringField(out, "payload", "RemotePlayResolvedLibraryTrack");
            out << ",\"track_id\":" << data.local_track_id;
            appendStringField(out, "source", data.source);
        }
    }, payload.data);
}

RuntimeCommandPayload parsePayload(std::string_view json)
{
    const auto payload = stringField(json, "payload").value_or("Empty");
    if (payload == "PlaybackStartTrack") {
        return RuntimeCommandPayload::startTrack(numberField<int>(json, "track_id").value_or(0));
    }
    if (payload == "PlaybackSeek") {
        return RuntimeCommandPayload::seek(numberField<double>(json, "seconds").value_or(0.0));
    }
    if (payload == "QueueStep") {
        return RuntimeCommandPayload::queueStep(numberField<int>(json, "delta").value_or(0));
    }
    if (payload == "QueueIndex") {
        return RuntimeCommandPayload::queueIndex(numberField<int>(json, "queue_index").value_or(0));
    }
    if (payload == "RuntimeEnabled") {
        return RuntimeCommandPayload::enabled(boolField(json, "enabled").value_or(false));
    }
    if (payload == "EqSetBand") {
        return RuntimeCommandPayload::eqSetBand(numberField<int>(json, "eq_band_index").value_or(0), numberField<int>(json, "eq_gain_db").value_or(0));
    }
    if (payload == "EqAdjustBand") {
        return RuntimeCommandPayload::eqAdjustBand(numberField<int>(json, "eq_band_index").value_or(0), numberField<int>(json, "eq_gain_delta").value_or(0));
    }
    if (payload == "EqCyclePreset") {
        return RuntimeCommandPayload::eqCyclePreset(numberField<int>(json, "preset_delta").value_or(0));
    }
    if (payload == "EqApplyPreset") {
        return RuntimeCommandPayload::eqApplyPreset(stringField(json, "preset_name").value_or(std::string{}));
    }
    if (payload == "SettingsApplyLive") {
        return RuntimeCommandPayload::settingsApplyLive(
            stringField(json, "output_mode").value_or(std::string{}),
            stringField(json, "network_policy").value_or(std::string{}),
            stringField(json, "sleep_timer").value_or(std::string{}));
    }
    return RuntimeCommandPayload::empty();
}

void appendResult(std::ostringstream& out, const RuntimeCommandResult& result)
{
    appendBoolField(out, "accepted", result.accepted);
    appendBoolField(out, "applied", result.applied);
    appendStringField(out, "code", result.code);
    appendStringField(out, "message", result.message);
    appendStringField(out, "origin", commandOriginName(result.origin));
    appendStringField(out, "correlation_id", result.correlation_id);
    out << ",\"version_before_apply\":" << result.version_before_apply
        << ",\"version_after_apply\":" << result.version_after_apply;
}

void appendSnapshot(std::ostringstream& out, const RuntimeSnapshot& snapshot)
{
    out << ",\"version\":" << snapshot.version;
    appendStringField(out, "playback_status", snapshot.playback.status == RuntimePlaybackStatus::Playing ? "Playing" : (snapshot.playback.status == RuntimePlaybackStatus::Paused ? "Paused" : "Empty"));
    out << ",\"current_track_id\":" << snapshot.playback.current_track_id.value_or(0);
    appendStringField(out, "title", snapshot.playback.title);
    appendStringField(out, "source_label", snapshot.playback.source_label);
    out << ",\"elapsed_seconds\":" << snapshot.playback.elapsed_seconds
        << ",\"duration_seconds\":" << snapshot.playback.duration_seconds;
    appendBoolField(out, "audio_active", snapshot.playback.audio_active);
    appendBoolField(out, "shuffle_enabled", snapshot.playback.shuffle_enabled);
    appendBoolField(out, "repeat_all", snapshot.playback.repeat_all);
    appendBoolField(out, "repeat_one", snapshot.playback.repeat_one);
    appendIntArray(out, "queue_active_ids", snapshot.queue.active_ids);
    out << ",\"queue_active_index\":" << snapshot.queue.active_index;
    appendIntArray(out, "eq_bands", snapshot.eq.bands);
    appendStringField(out, "eq_preset_name", snapshot.eq.preset_name);
    appendBoolField(out, "eq_enabled", snapshot.eq.enabled);
    appendStringField(out, "remote_profile_id", snapshot.remote.profile_id);
    appendStringField(out, "remote_source_label", snapshot.remote.source_label);
    appendStringField(out, "remote_connection_status", snapshot.remote.connection_status);
    appendBoolField(out, "remote_stream_resolved", snapshot.remote.stream_resolved);
    appendStringField(out, "remote_redacted_url", snapshot.remote.redacted_url);
    appendStringField(out, "remote_buffer_state", snapshot.remote.buffer_state);
    appendStringField(out, "remote_recovery_action", snapshot.remote.recovery_action);
    out << ",\"remote_bitrate_kbps\":" << snapshot.remote.bitrate_kbps;
    appendStringField(out, "remote_codec", snapshot.remote.codec);
    appendBoolField(out, "remote_live", snapshot.remote.live);
    appendBoolField(out, "remote_seekable", snapshot.remote.seekable);
    appendStringField(out, "settings_output_mode", snapshot.settings.output_mode);
    appendStringField(out, "settings_network_policy", snapshot.settings.network_policy);
    appendStringField(out, "settings_sleep_timer", snapshot.settings.sleep_timer);
}

RuntimeSnapshot parseSnapshot(std::string_view json)
{
    RuntimeSnapshot snapshot{};
    snapshot.version = numberField<std::uint64_t>(json, "version").value_or(0);
    const auto status = stringField(json, "playback_status").value_or("Empty");
    snapshot.playback.status = status == "Playing" ? RuntimePlaybackStatus::Playing : (status == "Paused" ? RuntimePlaybackStatus::Paused : RuntimePlaybackStatus::Empty);
    if (const auto track_id = numberField<int>(json, "current_track_id"); track_id && *track_id > 0) {
        snapshot.playback.current_track_id = *track_id;
    }
    snapshot.playback.title = stringField(json, "title").value_or(std::string{});
    snapshot.playback.source_label = stringField(json, "source_label").value_or(std::string{});
    snapshot.playback.elapsed_seconds = numberField<double>(json, "elapsed_seconds").value_or(0.0);
    snapshot.playback.duration_seconds = numberField<int>(json, "duration_seconds").value_or(0);
    snapshot.playback.audio_active = boolField(json, "audio_active").value_or(false);
    snapshot.playback.shuffle_enabled = boolField(json, "shuffle_enabled").value_or(false);
    snapshot.playback.repeat_all = boolField(json, "repeat_all").value_or(false);
    snapshot.playback.repeat_one = boolField(json, "repeat_one").value_or(false);
    snapshot.queue.active_ids = intArrayField(json, "queue_active_ids");
    snapshot.queue.active_index = numberField<int>(json, "queue_active_index").value_or(-1);
    const auto bands = intArrayField(json, "eq_bands");
    for (std::size_t index = 0; index < snapshot.eq.bands.size() && index < bands.size(); ++index) {
        snapshot.eq.bands[index] = bands[index];
    }
    snapshot.eq.preset_name = stringField(json, "eq_preset_name").value_or("FLAT");
    snapshot.eq.enabled = boolField(json, "eq_enabled").value_or(false);
    snapshot.remote.profile_id = stringField(json, "remote_profile_id").value_or(std::string{});
    snapshot.remote.source_label = stringField(json, "remote_source_label").value_or("REMOTE");
    snapshot.remote.connection_status = stringField(json, "remote_connection_status").value_or("UNKNOWN");
    snapshot.remote.stream_resolved = boolField(json, "remote_stream_resolved").value_or(false);
    snapshot.remote.redacted_url = stringField(json, "remote_redacted_url").value_or(std::string{});
    snapshot.remote.buffer_state = stringField(json, "remote_buffer_state").value_or("UNKNOWN");
    snapshot.remote.recovery_action = stringField(json, "remote_recovery_action").value_or("NONE");
    snapshot.remote.bitrate_kbps = numberField<int>(json, "remote_bitrate_kbps").value_or(0);
    snapshot.remote.codec = stringField(json, "remote_codec").value_or(std::string{});
    snapshot.remote.live = boolField(json, "remote_live").value_or(false);
    snapshot.remote.seekable = boolField(json, "remote_seekable").value_or(false);
    snapshot.settings.output_mode = stringField(json, "settings_output_mode").value_or("DEFAULT");
    snapshot.settings.network_policy = stringField(json, "settings_network_policy").value_or("DEFAULT");
    snapshot.settings.sleep_timer = stringField(json, "settings_sleep_timer").value_or("OFF");
    return snapshot;
}

} // namespace

std::string runtimeCommandKindName(RuntimeCommandKind kind) noexcept
{
    return std::string{enumName(kind, kCommandNames)};
}

std::optional<RuntimeCommandKind> runtimeCommandKindFromName(std::string_view name) noexcept
{
    return enumFromName(name, kCommandNames);
}

std::string runtimeQueryKindName(RuntimeQueryKind kind) noexcept
{
    return std::string{enumName(kind, kQueryNames)};
}

std::optional<RuntimeQueryKind> runtimeQueryKindFromName(std::string_view name) noexcept
{
    return enumFromName(name, kQueryNames);
}

std::string commandOriginName(CommandOrigin origin) noexcept
{
    return std::string{enumName(origin, kOriginNames)};
}

std::optional<CommandOrigin> commandOriginFromName(std::string_view name) noexcept
{
    return enumFromName(name, kOriginNames);
}

std::string serializeRuntimeCommandRequest(const RuntimeCommandRequest& request)
{
    std::ostringstream out;
    out << "{\"type\":\"command\"";
    appendStringField(out, "kind", runtimeCommandKindName(request.command.kind));
    appendStringField(out, "origin", commandOriginName(request.command.origin));
    appendStringField(out, "correlation_id", request.command.correlation_id);
    appendPayload(out, request.command.payload);
    out << '}';
    return out.str();
}

std::string serializeRuntimeCommandResponse(const RuntimeCommandResponse& response)
{
    std::ostringstream out;
    out << "{\"type\":\"command_response\"";
    appendResult(out, response.result);
    out << '}';
    return out.str();
}

std::string serializeRuntimeQueryRequest(const RuntimeQueryRequest& request)
{
    std::ostringstream out;
    out << "{\"type\":\"query\"";
    appendStringField(out, "kind", runtimeQueryKindName(request.query.kind));
    appendStringField(out, "origin", commandOriginName(request.query.origin));
    appendStringField(out, "correlation_id", request.query.correlation_id);
    out << '}';
    return out.str();
}

std::string serializeRuntimeQueryResponse(const RuntimeQueryResponse& response)
{
    std::ostringstream out;
    out << "{\"type\":\"query_response\"";
    appendSnapshot(out, response.snapshot);
    out << '}';
    return out.str();
}

std::optional<RuntimeCommandRequest> parseRuntimeCommandRequest(std::string_view json)
{
    const auto kind_name = stringField(json, "kind");
    const auto kind = kind_name ? runtimeCommandKindFromName(*kind_name) : std::nullopt;
    if (!kind) {
        return std::nullopt;
    }
    const auto origin_name = stringField(json, "origin");
    return RuntimeCommandRequest{RuntimeCommand{
        *kind,
        parsePayload(json),
        origin_name ? commandOriginFromName(*origin_name).value_or(CommandOrigin::Unknown) : CommandOrigin::Unknown,
        stringField(json, "correlation_id").value_or(std::string{})}};
}

std::optional<RuntimeCommandResponse> parseRuntimeCommandResponse(std::string_view json)
{
    RuntimeCommandResult result{};
    result.accepted = boolField(json, "accepted").value_or(false);
    result.applied = boolField(json, "applied").value_or(false);
    result.code = stringField(json, "code").value_or(std::string{});
    result.message = stringField(json, "message").value_or(std::string{});
    if (const auto origin = stringField(json, "origin")) {
        result.origin = commandOriginFromName(*origin).value_or(CommandOrigin::Unknown);
    }
    result.correlation_id = stringField(json, "correlation_id").value_or(std::string{});
    result.version_before_apply = numberField<std::uint64_t>(json, "version_before_apply").value_or(0);
    result.version_after_apply = numberField<std::uint64_t>(json, "version_after_apply").value_or(0);
    return RuntimeCommandResponse{result};
}

std::optional<RuntimeQueryRequest> parseRuntimeQueryRequest(std::string_view json)
{
    const auto kind_name = stringField(json, "kind");
    const auto kind = kind_name ? runtimeQueryKindFromName(*kind_name) : std::nullopt;
    if (!kind) {
        return std::nullopt;
    }
    const auto origin_name = stringField(json, "origin");
    return RuntimeQueryRequest{RuntimeQuery{
        *kind,
        origin_name ? commandOriginFromName(*origin_name).value_or(CommandOrigin::Unknown) : CommandOrigin::Unknown,
        stringField(json, "correlation_id").value_or(std::string{})}};
}

std::optional<RuntimeQueryResponse> parseRuntimeQueryResponse(std::string_view json)
{
    return RuntimeQueryResponse{parseSnapshot(json)};
}

} // namespace lofibox::runtime
