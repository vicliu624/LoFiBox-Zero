// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace lofibox::playlist {

enum class PlaylistFormat {
    Unknown,
    M3u,
    Pls,
    Xspf,
};

struct PlaylistEntry {
    std::string location{};
    std::string title{};
    int duration_seconds{-1};
};

struct PlaylistDocument {
    PlaylistFormat format{PlaylistFormat::Unknown};
    std::vector<PlaylistEntry> entries{};
};

[[nodiscard]] PlaylistFormat detectPlaylistFormat(const std::filesystem::path& path);
[[nodiscard]] PlaylistDocument parseM3u(std::string_view text);

} // namespace lofibox::playlist
