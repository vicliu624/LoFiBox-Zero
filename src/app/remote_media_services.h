// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace lofibox::app {

enum class RemoteServerKind {
    Jellyfin,
    OpenSubsonic,
    Emby,
};

struct RemoteServerProfile {
    RemoteServerKind kind{RemoteServerKind::Jellyfin};
    std::string id{};
    std::string name{};
    std::string base_url{};
    std::string username{};
    std::string password{};
    std::string api_token{};
};

struct RemoteSourceSession {
    bool available{false};
    std::string server_name{};
    std::string user_id{};
    std::string access_token{};
    std::string message{};
};

struct RemoteTrack {
    std::string id{};
    std::string title{};
    std::string artist{};
    std::string album{};
    std::string album_id{};
    int duration_seconds{0};
};

struct ResolvedRemoteStream {
    std::string url{};
    std::vector<std::pair<std::string, std::string>> headers{};
    bool seekable{true};
};

class RemoteSourceProvider {
public:
    virtual ~RemoteSourceProvider() = default;

    [[nodiscard]] virtual bool available() const = 0;
    [[nodiscard]] virtual std::string displayName() const = 0;
    [[nodiscard]] virtual RemoteSourceSession probe(const RemoteServerProfile& profile) const = 0;
};

class RemoteCatalogProvider {
public:
    virtual ~RemoteCatalogProvider() = default;

    [[nodiscard]] virtual bool available() const = 0;
    [[nodiscard]] virtual std::string displayName() const = 0;
    [[nodiscard]] virtual std::vector<RemoteTrack> searchTracks(
        const RemoteServerProfile& profile,
        const RemoteSourceSession& session,
        std::string_view query,
        int limit) const = 0;
    [[nodiscard]] virtual std::vector<RemoteTrack> recentTracks(
        const RemoteServerProfile& profile,
        const RemoteSourceSession& session,
        int limit) const = 0;
};

class RemoteStreamResolver {
public:
    virtual ~RemoteStreamResolver() = default;

    [[nodiscard]] virtual bool available() const = 0;
    [[nodiscard]] virtual std::string displayName() const = 0;
    [[nodiscard]] virtual std::optional<ResolvedRemoteStream> resolveTrack(
        const RemoteServerProfile& profile,
        const RemoteSourceSession& session,
        const RemoteTrack& track) const = 0;
};

} // namespace lofibox::app
