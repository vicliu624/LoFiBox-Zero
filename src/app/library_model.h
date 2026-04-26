// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace lofibox::app {

inline constexpr std::string_view kUnknownMetadata = "UNKNOWN";

enum class LibraryIndexState {
    Uninitialized,
    Loading,
    Ready,
    Degraded,
};

enum class AlbumsMode {
    All,
    ByArtist,
};

enum class SongsMode {
    All,
    Album,
    Genre,
    Composer,
    Compilation,
};

enum class PlaylistKind {
    OnTheGo,
    RecentlyAdded,
    MostPlayed,
    RecentlyPlayed,
};

enum class SongSortMode {
    TitleAscending,
    TitleDescending,
    ArtistAscending,
    ArtistDescending,
    AddedAscending,
    AddedDescending,
};

struct StorageInfo {
    bool available{false};
    std::uintmax_t capacity_bytes{0};
    std::uintmax_t free_bytes{0};
};

struct TrackRecord {
    int id{0};
    std::filesystem::path path{};
    std::string title{std::string(kUnknownMetadata)};
    std::string artist{std::string(kUnknownMetadata)};
    std::string album{std::string(kUnknownMetadata)};
    std::string genre{std::string(kUnknownMetadata)};
    std::string composer{std::string(kUnknownMetadata)};
    std::int64_t added_time{0};
    int duration_seconds{180};
    int play_count{0};
    std::int64_t last_played{0};
};

struct AlbumRecord {
    std::string album{};
    std::string artist{};
    std::vector<int> track_ids{};
};

struct CompilationRecord {
    std::string album{};
    std::vector<int> track_ids{};
};

struct LibraryModel {
    std::vector<TrackRecord> tracks{};
    std::vector<std::string> artists{};
    std::vector<AlbumRecord> albums{};
    std::vector<std::string> genres{};
    std::vector<std::string> composers{};
    std::vector<CompilationRecord> compilations{};
    StorageInfo storage{};
    bool degraded{false};
};

struct AlbumsContext {
    AlbumsMode mode{AlbumsMode::All};
    std::string artist{};
};

struct SongsContext {
    SongsMode mode{SongsMode::All};
    std::string label{};
    std::vector<int> track_ids{};
};

struct PlaylistContext {
    PlaylistKind kind{PlaylistKind::OnTheGo};
    std::string name{};
};

} // namespace lofibox::app
