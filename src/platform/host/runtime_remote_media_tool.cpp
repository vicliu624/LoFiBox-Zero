// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/runtime_remote_media_tool.h"

#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "platform/host/runtime_host_internal.h"
#include "platform/host/runtime_logger.h"

namespace fs = std::filesystem;

namespace lofibox::platform::host::runtime_detail {
namespace {

std::string toKindString(app::RemoteServerKind kind)
{
    switch (kind) {
    case app::RemoteServerKind::Jellyfin: return "jellyfin";
    case app::RemoteServerKind::OpenSubsonic: return "opensubsonic";
    case app::RemoteServerKind::Emby: return "emby";
    }
    return "jellyfin";
}

std::string remoteProfileJson(const app::RemoteServerProfile& profile)
{
    std::ostringstream out;
    out << "{"
        << "\"kind\":\"" << toKindString(profile.kind) << "\","
        << "\"id\":\"" << jsonEscape(profile.id) << "\","
        << "\"name\":\"" << jsonEscape(profile.name) << "\","
        << "\"base_url\":\"" << jsonEscape(profile.base_url) << "\","
        << "\"username\":\"" << jsonEscape(profile.username) << "\","
        << "\"password\":\"" << jsonEscape(profile.password) << "\","
        << "\"api_token\":\"" << jsonEscape(profile.api_token) << "\""
        << "}";
    return out.str();
}

std::string remoteSessionJson(const app::RemoteSourceSession& session)
{
    std::ostringstream out;
    out << "{\"available\":" << (session.available ? "true" : "false")
        << ",\"server_name\":\"" << jsonEscape(session.server_name)
        << "\",\"user_id\":\"" << jsonEscape(session.user_id)
        << "\",\"access_token\":\"" << jsonEscape(session.access_token)
        << "\",\"message\":\"" << jsonEscape(session.message)
        << "\"}";
    return out.str();
}

app::RemoteSourceSession parseRemoteSession(std::string_view json)
{
    app::RemoteSourceSession session{};
    if (const auto value = extractJsonString(json, "\"server_name\":\"")) session.server_name = *value;
    if (const auto value = extractJsonString(json, "\"user_id\":\"")) session.user_id = *value;
    if (const auto value = extractJsonString(json, "\"access_token\":\"")) session.access_token = *value;
    if (const auto value = extractJsonString(json, "\"message\":\"")) session.message = *value;
    session.available = parseJsonBool(json, "\"available\":").value_or(false);
    return session;
}

std::vector<app::RemoteTrack> parseRemoteTracks(std::string_view json)
{
    std::vector<app::RemoteTrack> tracks{};
    std::size_t pos = json.find("\"tracks\":[");
    if (pos == std::string_view::npos) {
        return tracks;
    }

    pos = json.find('{', pos);
    while (pos != std::string_view::npos) {
        const auto end = json.find('}', pos);
        if (end == std::string_view::npos) {
            break;
        }
        const auto block = json.substr(pos, end - pos + 1);
        app::RemoteTrack track{};
        if (const auto value = extractJsonString(block, "\"id\":\"")) track.id = *value;
        if (const auto value = extractJsonString(block, "\"title\":\"")) track.title = *value;
        if (const auto value = extractJsonString(block, "\"artist\":\"")) track.artist = *value;
        if (const auto value = extractJsonString(block, "\"album\":\"")) track.album = *value;
        if (const auto value = extractJsonString(block, "\"album_id\":\"")) track.album_id = *value;
        if (const auto duration = parseJsonInt(block, "\"duration_seconds\":")) {
            track.duration_seconds = *duration;
        }
        if (!track.id.empty()) {
            tracks.push_back(track);
        }
        pos = json.find('{', end + 1);
    }
    return tracks;
}

std::optional<app::ResolvedRemoteStream> parseResolvedStream(std::string_view json)
{
    app::ResolvedRemoteStream stream{};
    if (const auto url = extractJsonString(json, "\"url\":\"")) {
        stream.url = *url;
    } else {
        return std::nullopt;
    }
    stream.seekable = parseJsonBool(json, "\"seekable\":").value_or(true);
    return stream;
}

} // namespace

RemoteMediaToolClient::RemoteMediaToolClient()
{
    python_path_ = resolvePythonPath();
}

bool RemoteMediaToolClient::available() const
{
    return python_path_.has_value() && fs::exists(remoteMediaToolScriptPath());
}

app::RemoteSourceSession RemoteMediaToolClient::probe(const app::RemoteServerProfile& profile) const
{
    if (!available()) {
        logRuntime(RuntimeLogLevel::Warn, "remote", "Remote source provider unavailable");
        return {};
    }
    const std::string payload = std::string("{\"action\":\"probe\",\"profile\":") + remoteProfileJson(profile) + "}";
    if (const auto json = callRemoteMediaTool(*python_path_, payload)) {
        auto session = parseRemoteSession(*json);
        logRuntime(session.available ? RuntimeLogLevel::Info : RuntimeLogLevel::Warn, "remote", "Probe " + profile.name + " -> " + (session.available ? "available" : "unavailable"));
        return session;
    }
    logRuntime(RuntimeLogLevel::Warn, "remote", "Probe failed for " + profile.name);
    return {};
}

std::vector<app::RemoteTrack> RemoteMediaToolClient::searchTracks(
    const app::RemoteServerProfile& profile,
    const app::RemoteSourceSession& session,
    std::string_view query,
    int limit) const
{
    if (!available()) {
        logRuntime(RuntimeLogLevel::Warn, "remote", "Remote catalog provider unavailable");
        return {};
    }
    std::ostringstream payload;
    payload << "{\"action\":\"search\",\"profile\":" << remoteProfileJson(profile)
            << ",\"session\":" << remoteSessionJson(session)
            << ",\"query\":\"" << jsonEscape(std::string(query))
            << "\",\"limit\":" << limit << "}";
    if (const auto json = callRemoteMediaTool(*python_path_, payload.str())) {
        auto tracks = parseRemoteTracks(*json);
        logRuntime(RuntimeLogLevel::Info, "remote", "Search " + profile.name + " returned " + std::to_string(tracks.size()) + " tracks");
        return tracks;
    }
    logRuntime(RuntimeLogLevel::Warn, "remote", "Search failed for " + profile.name);
    return {};
}

std::vector<app::RemoteTrack> RemoteMediaToolClient::recentTracks(
    const app::RemoteServerProfile& profile,
    const app::RemoteSourceSession& session,
    int limit) const
{
    if (!available()) {
        logRuntime(RuntimeLogLevel::Warn, "remote", "Remote catalog provider unavailable");
        return {};
    }
    std::ostringstream payload;
    payload << "{\"action\":\"recent\",\"profile\":" << remoteProfileJson(profile)
            << ",\"session\":" << remoteSessionJson(session)
            << ",\"limit\":" << limit << "}";
    if (const auto json = callRemoteMediaTool(*python_path_, payload.str())) {
        auto tracks = parseRemoteTracks(*json);
        logRuntime(RuntimeLogLevel::Info, "remote", "Recent " + profile.name + " returned " + std::to_string(tracks.size()) + " tracks");
        return tracks;
    }
    logRuntime(RuntimeLogLevel::Warn, "remote", "Recent query failed for " + profile.name);
    return {};
}

std::optional<app::ResolvedRemoteStream> RemoteMediaToolClient::resolveTrack(
    const app::RemoteServerProfile& profile,
    const app::RemoteSourceSession& session,
    const app::RemoteTrack& track) const
{
    if (!available()) {
        logRuntime(RuntimeLogLevel::Warn, "remote", "Remote stream resolver unavailable");
        return std::nullopt;
    }
    std::ostringstream payload;
    payload << "{\"action\":\"resolve\",\"profile\":" << remoteProfileJson(profile)
            << ",\"session\":" << remoteSessionJson(session)
            << ",\"track\":{\"id\":\"" << jsonEscape(track.id)
            << "\",\"title\":\"" << jsonEscape(track.title)
            << "\",\"artist\":\"" << jsonEscape(track.artist)
            << "\",\"album\":\"" << jsonEscape(track.album)
            << "\",\"album_id\":\"" << jsonEscape(track.album_id)
            << "\",\"duration_seconds\":" << track.duration_seconds
            << "}}";
    if (const auto json = callRemoteMediaTool(*python_path_, payload.str())) {
        auto resolved = parseResolvedStream(*json);
        logRuntime(resolved ? RuntimeLogLevel::Info : RuntimeLogLevel::Warn, "remote", std::string("Resolve ") + profile.name + " track " + track.id + (resolved ? " succeeded" : " failed"));
        return resolved;
    }
    logRuntime(RuntimeLogLevel::Warn, "remote", "Resolve failed for " + profile.name + " track " + track.id);
    return std::nullopt;
}

} // namespace lofibox::platform::host::runtime_detail
