// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "app/remote_media_services.h"
#include "app/runtime_services.h"
#include "application/app_command_result.h"

namespace lofibox::application {

struct RemoteBrowseQueryResult {
    CommandResult command{};
    app::RemoteServerProfile profile{};
    app::RemoteSourceSession session{};
    app::RemoteCatalogNode parent{};
    std::vector<app::RemoteCatalogNode> nodes{};
    std::vector<app::RemoteTrack> playable_tracks{};
    std::string source_label{};
    bool online{false};
    bool degraded{false};
    bool from_cache{false};
    std::string degraded_reason{};
};

struct RemoteSearchQueryResult {
    CommandResult command{};
    app::RemoteServerProfile profile{};
    app::RemoteSourceSession session{};
    std::vector<app::RemoteTrack> tracks{};
    std::string source_label{};
    bool degraded{false};
    std::string degraded_reason{};
};

struct RemoteResolveQueryResult {
    CommandResult command{};
    app::RemoteServerProfile profile{};
    app::RemoteSourceSession session{};
    app::RemoteTrack track{};
    std::optional<app::ResolvedRemoteStream> stream{};
};

struct RemoteSourceDiagnostics {
    std::string source_label{};
    std::string kind_id{};
    std::string family{};
    std::string connection_status{};
    std::string message{};
    std::string user{};
    std::string credential_status{};
    std::string tls_status{};
    std::string permission{};
    std::string token_status{};
    bool provider_available{false};
    bool catalog_available{false};
    bool resolver_available{false};
    bool session_available{false};
    bool credential_attached{false};
    std::vector<std::string> capabilities{};
};

struct RemoteStreamDiagnosticsQuery {
    bool resolved{false};
    std::string source_name{};
    std::string redacted_url{};
    std::string connection_status{};
    std::string buffer_state{};
    int minimum_playable_seconds{0};
    std::string recovery_action{};
    app::StreamQualityPreference quality_preference{app::StreamQualityPreference::Auto};
    int bitrate_kbps{0};
    std::string codec{};
    int sample_rate_hz{0};
    int channel_count{0};
    bool live{false};
    bool seekable{true};
};

class RemoteBrowseQueryService {
public:
    explicit RemoteBrowseQueryService(const app::RuntimeServices& services) noexcept;

    [[nodiscard]] RemoteBrowseQueryResult browseRoot(app::RemoteServerProfile& profile, std::size_t profile_count, int limit) const;
    [[nodiscard]] RemoteBrowseQueryResult browseChild(
        app::RemoteServerProfile& profile,
        const app::RemoteSourceSession& session,
        const app::RemoteCatalogNode& parent,
        int limit) const;
    [[nodiscard]] RemoteBrowseQueryResult libraryTracks(app::RemoteServerProfile& profile, std::size_t profile_count, int limit) const;
    [[nodiscard]] RemoteSearchQueryResult search(app::RemoteServerProfile& profile, std::size_t profile_count, std::string_view query, int limit) const;
    [[nodiscard]] RemoteResolveQueryResult resolve(
        app::RemoteServerProfile& profile,
        const app::RemoteSourceSession& session,
        const app::RemoteTrack& track) const;

    [[nodiscard]] app::RemoteTrack trackFromNode(const app::RemoteServerProfile& profile, const app::RemoteCatalogNode& node) const;
    [[nodiscard]] std::vector<app::RemoteTrack> tracksFromPlayableNodes(
        const app::RemoteServerProfile& profile,
        const std::vector<app::RemoteCatalogNode>& nodes) const;
    void rememberRecentBrowse(const app::RemoteServerProfile& profile, const app::RemoteCatalogNode& node) const;
    void rememberTrackFacts(const app::RemoteServerProfile& profile, const app::RemoteTrack& track) const;

    [[nodiscard]] RemoteSourceDiagnostics sourceDiagnostics(
        const std::optional<app::RemoteServerProfile>& profile,
        const app::RemoteSourceSession& session) const;
    [[nodiscard]] RemoteStreamDiagnosticsQuery streamDiagnostics(
        const std::optional<app::RemoteServerProfile>& profile,
        const std::optional<app::ResolvedRemoteStream>& stream) const;

private:
    [[nodiscard]] std::string sourceLabel(const app::RemoteServerProfile& profile) const;
    [[nodiscard]] bool keepsLocalFacts(app::RemoteServerKind kind) const;
    [[nodiscard]] std::vector<app::RemoteCatalogNode> cachedBrowse(
        const app::RemoteServerProfile& profile,
        const app::RemoteCatalogNode& parent) const;
    void rememberBrowse(
        const app::RemoteServerProfile& profile,
        const app::RemoteCatalogNode& parent,
        const std::vector<app::RemoteCatalogNode>& nodes) const;
    [[nodiscard]] app::RemoteTrack mergeCachedTrackFacts(
        const app::RemoteServerProfile& profile,
        app::RemoteTrack track) const;

    const app::RuntimeServices& services_;
};

} // namespace lofibox::application
