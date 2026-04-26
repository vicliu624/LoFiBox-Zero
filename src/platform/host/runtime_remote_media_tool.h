// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "app/remote_media_services.h"

namespace lofibox::platform::host::runtime_detail {
namespace fs = std::filesystem;

class RemoteMediaToolClient {
public:
    RemoteMediaToolClient();
    [[nodiscard]] bool available() const;
    [[nodiscard]] app::RemoteSourceSession probe(const app::RemoteServerProfile& profile) const;
    [[nodiscard]] std::vector<app::RemoteTrack> searchTracks(
        const app::RemoteServerProfile& profile,
        const app::RemoteSourceSession& session,
        std::string_view query,
        int limit) const;
    [[nodiscard]] std::vector<app::RemoteTrack> recentTracks(
        const app::RemoteServerProfile& profile,
        const app::RemoteSourceSession& session,
        int limit) const;
    [[nodiscard]] std::vector<app::RemoteTrack> libraryTracks(
        const app::RemoteServerProfile& profile,
        const app::RemoteSourceSession& session,
        int limit) const;
    [[nodiscard]] std::optional<app::ResolvedRemoteStream> resolveTrack(
        const app::RemoteServerProfile& profile,
        const app::RemoteSourceSession& session,
        const app::RemoteTrack& track) const;

private:
    std::optional<fs::path> python_path_{};
};

} // namespace lofibox::platform::host::runtime_detail
