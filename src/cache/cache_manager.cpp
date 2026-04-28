// SPDX-License-Identifier: GPL-3.0-or-later

#include "cache/cache_manager.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <system_error>
#include <unordered_map>

namespace lofibox::cache {
namespace fs = std::filesystem;
namespace {

std::string bucketName(CacheBucket bucket)
{
    switch (bucket) {
    case CacheBucket::PlaybackBuffer:
        return "playback";
    case CacheBucket::RecentItem:
        return "recent";
    case CacheBucket::Artwork:
        return "artwork";
    case CacheBucket::Metadata:
        return "metadata";
    case CacheBucket::Lyrics:
        return "lyrics";
    case CacheBucket::RemoteDirectory:
        return "remote-directory";
    case CacheBucket::StationDirectory:
        return "stations";
    case CacheBucket::OfflineAudio:
        return "offline";
    }
    return "unknown";
}

std::chrono::system_clock::time_point fileTimeToSystem(fs::file_time_type value)
{
    const auto now_file = fs::file_time_type::clock::now();
    const auto now_system = std::chrono::system_clock::now();
    return now_system + std::chrono::duration_cast<std::chrono::system_clock::duration>(value - now_file);
}

} // namespace

CacheManager::CacheManager(fs::path root)
    : root_(std::move(root))
{
    policies_ = {
        {CacheBucket::PlaybackBuffer, CachePolicy{64ULL * 1024ULL * 1024ULL, std::chrono::hours{24}}},
        {CacheBucket::RecentItem, CachePolicy{4ULL * 1024ULL * 1024ULL, std::chrono::hours{24 * 30}}},
        {CacheBucket::Artwork, CachePolicy{128ULL * 1024ULL * 1024ULL, std::chrono::hours{24 * 180}}},
        {CacheBucket::Metadata, CachePolicy{32ULL * 1024ULL * 1024ULL, std::chrono::hours{24 * 180}}},
        {CacheBucket::Lyrics, CachePolicy{16ULL * 1024ULL * 1024ULL, std::chrono::hours{24 * 180}}},
        {CacheBucket::RemoteDirectory, CachePolicy{32ULL * 1024ULL * 1024ULL, std::chrono::hours{24 * 14}}},
        {CacheBucket::StationDirectory, CachePolicy{8ULL * 1024ULL * 1024ULL, std::chrono::hours{24 * 30}}},
        {CacheBucket::OfflineAudio, CachePolicy{2ULL * 1024ULL * 1024ULL * 1024ULL, std::chrono::hours{24 * 365 * 10}}},
    };
    std::error_code ec{};
    fs::create_directories(root_, ec);
}

const fs::path& CacheManager::root() const noexcept
{
    return root_;
}

void CacheManager::setPolicy(CacheBucket bucket, CachePolicy policy)
{
    auto found = std::find_if(policies_.begin(), policies_.end(), [&](const auto& entry) {
        return entry.first == bucket;
    });
    if (found == policies_.end()) {
        policies_.push_back({bucket, policy});
    } else {
        found->second = policy;
    }
}

CachePolicy CacheManager::policy(CacheBucket bucket) const
{
    const auto found = std::find_if(policies_.begin(), policies_.end(), [&](const auto& entry) {
        return entry.first == bucket;
    });
    return found == policies_.end() ? CachePolicy{} : found->second;
}

fs::path CacheManager::bucketPath(CacheBucket bucket) const
{
    return root_ / bucketName(bucket);
}

std::string CacheManager::safeKey(std::string_view value)
{
    std::string result{};
    result.reserve(value.size());
    for (const unsigned char ch : value) {
        if (std::isalnum(ch) || ch == '-' || ch == '_') {
            result.push_back(static_cast<char>(ch));
        } else {
            result.push_back('_');
        }
    }
    if (result.empty()) {
        return "empty";
    }
    return result;
}

fs::path CacheManager::pathFor(CacheBucket bucket, std::string_view key, std::string_view extension) const
{
    return bucketPath(bucket) / (safeKey(key) + std::string(extension));
}

fs::path CacheManager::metadataPath(CacheBucket bucket, std::string_view key) const
{
    return bucketPath(bucket) / (safeKey(key) + ".meta");
}

bool CacheManager::putText(CacheBucket bucket, std::string_view key, std::string_view text, std::chrono::seconds ttl)
{
    std::error_code ec{};
    fs::create_directories(bucketPath(bucket), ec);
    std::ofstream output(pathFor(bucket, key, ".txt"), std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }
    output << text;
    output.close();
    if (ttl != std::chrono::seconds::zero()) {
        std::ofstream metadata(metadataPath(bucket, key), std::ios::binary | std::ios::trunc);
        metadata << "ttl=" << ttl.count() << '\n';
    }
    return true;
}

std::optional<std::string> CacheManager::getText(CacheBucket bucket, std::string_view key) const
{
    const auto path = pathFor(bucket, key, ".txt");
    if (expired(path, bucket)) {
        return std::nullopt;
    }
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        return std::nullopt;
    }
    std::ostringstream buffer{};
    buffer << input.rdbuf();
    return buffer.str();
}

bool CacheManager::putBinary(CacheBucket bucket, std::string_view key, const std::vector<std::uint8_t>& bytes, std::string_view extension)
{
    std::error_code ec{};
    fs::create_directories(bucketPath(bucket), ec);
    std::ofstream output(pathFor(bucket, key, extension), std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }
    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return true;
}

std::optional<std::vector<std::uint8_t>> CacheManager::getBinary(CacheBucket bucket, std::string_view key, std::string_view extension) const
{
    const auto path = pathFor(bucket, key, extension);
    if (expired(path, bucket)) {
        return std::nullopt;
    }
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        return std::nullopt;
    }
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input), {});
}

bool CacheManager::rememberRemoteDirectory(std::string_view source_id, std::string_view directory_key, std::string_view serialized_catalog)
{
    return putText(CacheBucket::RemoteDirectory, std::string(source_id) + "-" + std::string(directory_key), serialized_catalog);
}

std::optional<std::string> CacheManager::remoteDirectory(std::string_view source_id, std::string_view directory_key) const
{
    return getText(CacheBucket::RemoteDirectory, std::string(source_id) + "-" + std::string(directory_key));
}

bool CacheManager::rememberStationList(std::string_view source_id, std::string_view serialized_stations)
{
    return putText(CacheBucket::StationDirectory, source_id, serialized_stations);
}

std::optional<std::string> CacheManager::stationList(std::string_view source_id) const
{
    return getText(CacheBucket::StationDirectory, source_id);
}

bool CacheManager::rememberRecentBrowse(std::string_view source_id, std::string_view item_id, std::string_view display_label)
{
    const auto key = std::string(source_id) + "-recent";
    auto existing = getText(CacheBucket::RecentItem, key).value_or("");
    existing.insert(0, std::string(item_id) + "|" + std::string(display_label) + "\n");
    return putText(CacheBucket::RecentItem, key, existing);
}

std::vector<std::string> CacheManager::recentBrowseEntries(std::string_view source_id) const
{
    std::vector<std::string> result{};
    auto text = getText(CacheBucket::RecentItem, std::string(source_id) + "-recent");
    if (!text) {
        return result;
    }
    std::istringstream input{*text};
    std::string line{};
    while (std::getline(input, line)) {
        if (!line.empty()) {
            result.push_back(line);
        }
    }
    return result;
}

bool CacheManager::saveOfflineTrack(const OfflineSaveRequest& request, const std::vector<std::uint8_t>& audio_bytes)
{
    const auto key = std::string(request.source_id) + "-" + request.item_id;
    if (!putBinary(CacheBucket::OfflineAudio, key, audio_bytes, request.extension.empty() ? ".bin" : request.extension)) {
        return false;
    }
    std::ostringstream manifest{};
    manifest << "source_id=" << request.source_id << '\n';
    manifest << "item_id=" << request.item_id << '\n';
    manifest << "title=" << request.title << '\n';
    manifest << "album=" << request.album << '\n';
    manifest << "playlist=" << request.playlist << '\n';
    manifest << "extension=" << (request.extension.empty() ? ".bin" : request.extension) << '\n';
    return putText(CacheBucket::OfflineAudio, key + "-manifest", manifest.str());
}

std::optional<fs::path> CacheManager::offlineTrackPath(std::string_view source_id, std::string_view item_id) const
{
    const auto key = std::string(source_id) + "-" + std::string(item_id);
    const auto manifest = getText(CacheBucket::OfflineAudio, key + "-manifest");
    if (!manifest) {
        return std::nullopt;
    }
    std::string extension = ".bin";
    std::istringstream input{*manifest};
    std::string line{};
    while (std::getline(input, line)) {
        if (line.rfind("extension=", 0) == 0) {
            extension = line.substr(10);
        }
    }
    auto path = pathFor(CacheBucket::OfflineAudio, key, extension);
    std::error_code ec{};
    if (!fs::exists(path, ec) || ec) {
        return std::nullopt;
    }
    return path;
}

OfflineSyncPlan CacheManager::planAlbumSync(std::string source_id, std::string album, std::vector<OfflineSaveRequest> tracks) const
{
    for (auto& track : tracks) {
        track.source_id = source_id;
        track.album = album;
    }
    return OfflineSyncPlan{"album:" + album, std::move(tracks)};
}

OfflineSyncPlan CacheManager::planPlaylistSync(std::string source_id, std::string playlist, std::vector<OfflineSaveRequest> tracks) const
{
    for (auto& track : tracks) {
        track.source_id = source_id;
        track.playlist = playlist;
    }
    return OfflineSyncPlan{"playlist:" + playlist, std::move(tracks)};
}

CacheUsage CacheManager::usage() const
{
    CacheUsage result{};
    std::error_code ec{};
    if (!fs::exists(root_, ec) || ec) {
        return result;
    }
    for (const auto& entry : fs::recursive_directory_iterator(root_, ec)) {
        if (ec) {
            break;
        }
        if (!entry.is_regular_file(ec) || ec) {
            ec.clear();
            continue;
        }
        result.total_bytes += entry.file_size(ec);
        if (ec) {
            ec.clear();
        }
        ++result.entry_count;
    }
    return result;
}

bool CacheManager::expired(const fs::path& path, CacheBucket bucket) const
{
    std::error_code ec{};
    if (!fs::exists(path, ec) || ec) {
        return true;
    }
    const auto updated_at = fileTimeToSystem(fs::last_write_time(path, ec));
    if (ec) {
        return false;
    }
    return std::chrono::system_clock::now() - updated_at > policy(bucket).max_age;
}

std::size_t CacheManager::collectGarbage()
{
    std::vector<fs::path> removed{};
    std::error_code ec{};
    if (!fs::exists(root_, ec) || ec) {
        return 0;
    }
    for (const auto& entry : fs::recursive_directory_iterator(root_, ec)) {
        if (ec) {
            break;
        }
        if (!entry.is_regular_file(ec) || ec) {
            ec.clear();
            continue;
        }
        const auto parent = entry.path().parent_path().filename().string();
        CacheBucket bucket = CacheBucket::Metadata;
        for (const auto candidate : {CacheBucket::PlaybackBuffer, CacheBucket::RecentItem, CacheBucket::Artwork, CacheBucket::Metadata, CacheBucket::Lyrics, CacheBucket::RemoteDirectory, CacheBucket::StationDirectory, CacheBucket::OfflineAudio}) {
            if (bucketName(candidate) == parent) {
                bucket = candidate;
                break;
            }
        }
        if (bucket != CacheBucket::OfflineAudio && expired(entry.path(), bucket)) {
            fs::remove(entry.path(), ec);
            if (!ec) {
                removed.push_back(entry.path());
            }
            ec.clear();
        }
    }
    return removed.size();
}

std::size_t CacheManager::clearAll()
{
    std::size_t removed = 0;
    std::error_code ec{};
    if (!fs::exists(root_, ec) || ec) {
        return 0;
    }
    for (const auto& entry : fs::recursive_directory_iterator(root_, ec)) {
        if (ec) {
            break;
        }
        if (!entry.is_regular_file(ec) || ec) {
            ec.clear();
            continue;
        }
        fs::remove(entry.path(), ec);
        if (!ec) {
            ++removed;
        }
        ec.clear();
    }
    return removed;
}

} // namespace lofibox::cache
