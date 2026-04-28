// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace lofibox::cache {

enum class CacheBucket {
    PlaybackBuffer,
    RecentItem,
    Artwork,
    Metadata,
    Lyrics,
    RemoteDirectory,
    StationDirectory,
    OfflineAudio,
};

struct CachePolicy {
    std::uintmax_t max_bytes{256ULL * 1024ULL * 1024ULL};
    std::chrono::seconds max_age{std::chrono::hours{24 * 30}};
};

struct CacheEntry {
    CacheBucket bucket{CacheBucket::Metadata};
    std::string key{};
    std::filesystem::path path{};
    std::uintmax_t size_bytes{0};
    std::chrono::system_clock::time_point updated_at{};
    bool pinned{false};
    bool offline_playable{false};
};

struct CacheUsage {
    std::uintmax_t total_bytes{0};
    std::size_t entry_count{0};
};

struct OfflineSaveRequest {
    std::string source_id{};
    std::string item_id{};
    std::string title{};
    std::string album{};
    std::string playlist{};
    std::string extension{".bin"};
};

struct OfflineSyncPlan {
    std::string scope{};
    std::vector<OfflineSaveRequest> items{};
};

class CacheManager {
public:
    explicit CacheManager(std::filesystem::path root);

    [[nodiscard]] const std::filesystem::path& root() const noexcept;
    void setPolicy(CacheBucket bucket, CachePolicy policy);
    [[nodiscard]] CachePolicy policy(CacheBucket bucket) const;

    [[nodiscard]] std::filesystem::path pathFor(CacheBucket bucket, std::string_view key, std::string_view extension = ".cache") const;
    bool putText(CacheBucket bucket, std::string_view key, std::string_view text, std::chrono::seconds ttl = std::chrono::seconds::zero());
    [[nodiscard]] std::optional<std::string> getText(CacheBucket bucket, std::string_view key) const;
    bool putBinary(CacheBucket bucket, std::string_view key, const std::vector<std::uint8_t>& bytes, std::string_view extension = ".bin");
    [[nodiscard]] std::optional<std::vector<std::uint8_t>> getBinary(CacheBucket bucket, std::string_view key, std::string_view extension = ".bin") const;

    bool rememberRemoteDirectory(std::string_view source_id, std::string_view directory_key, std::string_view serialized_catalog);
    [[nodiscard]] std::optional<std::string> remoteDirectory(std::string_view source_id, std::string_view directory_key) const;
    bool rememberStationList(std::string_view source_id, std::string_view serialized_stations);
    [[nodiscard]] std::optional<std::string> stationList(std::string_view source_id) const;
    bool rememberRecentBrowse(std::string_view source_id, std::string_view item_id, std::string_view display_label);
    [[nodiscard]] std::vector<std::string> recentBrowseEntries(std::string_view source_id) const;

    bool saveOfflineTrack(const OfflineSaveRequest& request, const std::vector<std::uint8_t>& audio_bytes);
    [[nodiscard]] std::optional<std::filesystem::path> offlineTrackPath(std::string_view source_id, std::string_view item_id) const;
    [[nodiscard]] OfflineSyncPlan planAlbumSync(std::string source_id, std::string album, std::vector<OfflineSaveRequest> tracks) const;
    [[nodiscard]] OfflineSyncPlan planPlaylistSync(std::string source_id, std::string playlist, std::vector<OfflineSaveRequest> tracks) const;

    [[nodiscard]] CacheUsage usage() const;
    std::size_t collectGarbage();
    std::size_t clearAll();

private:
    [[nodiscard]] std::filesystem::path bucketPath(CacheBucket bucket) const;
    [[nodiscard]] std::filesystem::path metadataPath(CacheBucket bucket, std::string_view key) const;
    [[nodiscard]] static std::string safeKey(std::string_view value);
    [[nodiscard]] bool expired(const std::filesystem::path& path, CacheBucket bucket) const;

    std::filesystem::path root_{};
    std::vector<std::pair<CacheBucket, CachePolicy>> policies_{};
};

} // namespace lofibox::cache
