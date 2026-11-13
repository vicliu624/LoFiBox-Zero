// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/runtime_host_internal.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include "platform/host/runtime_paths.h"

namespace fs = std::filesystem;

namespace lofibox::platform::host::runtime_detail {
namespace {
constexpr int kMetadataCacheVersion = 5;

template <typename T>
void mergeField(T& current, const T& before, const T& after)
{
    if (after != before) current = after;
}

void mergeMetadata(app::TrackMetadata& current, const app::TrackMetadata& before, const app::TrackMetadata& after)
{
    mergeField(current.title, before.title, after.title);
    mergeField(current.artist, before.artist, after.artist);
    mergeField(current.album, before.album, after.album);
    mergeField(current.genre, before.genre, after.genre);
    mergeField(current.composer, before.composer, after.composer);
    mergeField(current.duration_seconds, before.duration_seconds, after.duration_seconds);
}

void mergeEntry(CacheEntry& current, const CacheEntry& before, const CacheEntry& after)
{
    mergeMetadata(current.metadata, before.metadata, after.metadata);
    mergeField(current.lyrics.plain, before.lyrics.plain, after.lyrics.plain);
    mergeField(current.lyrics.synced, before.lyrics.synced, after.lyrics.synced);
    mergeField(current.lyrics.source, before.lyrics.source, after.lyrics.source);
    mergeMetadata(current.identity.metadata, before.identity.metadata, after.identity.metadata);
    mergeField(current.identity.recording_mbid, before.identity.recording_mbid, after.identity.recording_mbid);
    mergeField(current.identity.release_mbid, before.identity.release_mbid, after.identity.release_mbid);
    mergeField(current.identity.release_group_mbid, before.identity.release_group_mbid, after.identity.release_group_mbid);
    mergeField(current.identity.fingerprint, before.identity.fingerprint, after.identity.fingerprint);
    mergeField(current.identity.source, before.identity.source, after.identity.source);
    mergeField(current.identity.confidence, before.identity.confidence, after.identity.confidence);
    mergeField(current.identity.found, before.identity.found, after.identity.found);
    mergeField(current.identity.audio_fingerprint_verified, before.identity.audio_fingerprint_verified, after.identity.audio_fingerprint_verified);
    mergeField(current.release_mbid, before.release_mbid, after.release_mbid);
    mergeField(current.release_group_mbid, before.release_group_mbid, after.release_group_mbid);
    mergeField(current.online_metadata_attempted, before.online_metadata_attempted, after.online_metadata_attempted);
    mergeField(current.online_artwork_attempted, before.online_artwork_attempted, after.online_artwork_attempted);
    mergeField(current.online_lyrics_attempted, before.online_lyrics_attempted, after.online_lyrics_attempted);
    mergeField(current.track_identity_attempted, before.track_identity_attempted, after.track_identity_attempted);
    mergeField(current.track_identity_version, before.track_identity_version, after.track_identity_version);
    mergeField(current.lyrics_lookup_version, before.lyrics_lookup_version, after.lyrics_lookup_version);
}
}

CacheEntrySession::CacheEntrySession(const SharedRuntimeCache& cache, std::string_view key)
    : cache_(cache), key_(key)
{
    {
        const std::lock_guard lock{cache_.entries_mutex_};
        if (const auto it = cache_.entries_.find(key_); it != cache_.entries_.end()) {
            base_ = entry = it->second;
            return;
        }
    }
    // File I/O is outside the entries lock, including the first disk read.
    auto loaded = loadMetadataCache(cache_, key_);
    const std::lock_guard lock{cache_.entries_mutex_};
    const auto [it, inserted] = cache_.entries_.try_emplace(key_, std::move(loaded));
    base_ = entry = it->second;
}

CacheEntrySession::~CacheEntrySession()
{
    commit();
}

void CacheEntrySession::commit()
{
    const std::lock_guard lock{cache_.entries_mutex_};
    auto& current = cache_.entries_.at(key_);
    mergeEntry(current, base_, entry);
    base_ = entry = current;
}

void CacheEntrySession::persist()
{
    // Serialize disk snapshots, but let memory-only readers/merges proceed.
    const std::lock_guard lock{cache_.file_mutex};
    commit();
    storeMetadataCache(cache_, key_, entry);
}

fs::path cacheRoot()
{
    return runtime_paths::appCacheDir();
}

SharedRuntimeCache::SharedRuntimeCache()
    : root(cacheRoot())
{
    std::error_code ec{};
    fs::create_directories(root / "artwork", ec);
    fs::create_directories(root / "metadata", ec);
}

std::string cacheKeyForPath(const fs::path& path)
{
    std::string identity = pathUtf8String(path);
    std::error_code ec{};
    if (fs::exists(path, ec) && !ec) {
        identity += "|size=" + std::to_string(static_cast<unsigned long long>(fs::file_size(path, ec)));
        if (ec) {
            ec.clear();
        }
        const auto write_time = fs::last_write_time(path, ec);
        if (!ec) {
            identity += "|mtime=" + std::to_string(write_time.time_since_epoch().count());
        }
    }
    return std::to_string(static_cast<unsigned long long>(std::hash<std::string>{}(identity)));
}

fs::path metadataCachePath(const SharedRuntimeCache& cache, std::string_view key)
{
    return cache.root / "metadata" / (std::string(key) + ".txt");
}

fs::path artworkCachePath(const SharedRuntimeCache& cache, std::string_view key)
{
    return cache.root / "artwork" / (std::string(key) + ".png");
}

fs::path artworkMissingMarkerPath(const SharedRuntimeCache& cache, std::string_view key)
{
    return cache.root / "artwork" / (std::string(key) + ".missing");
}

fs::path lyricsCachePath(const SharedRuntimeCache& cache, std::string_view key)
{
    return cache.root / "metadata" / (std::string(key) + ".lyrics.txt");
}

void storeMetadataCache(const SharedRuntimeCache& cache, std::string_view key, const CacheEntry& entry)
{
    const std::lock_guard lock{cache.file_mutex};
    std::ofstream output(metadataCachePath(cache, key), std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        return;
    }
    auto emitField = [&](std::string_view name, const std::optional<std::string>& value) {
        if (value) {
            output << name << '=' << *value << '\n';
        }
    };
    output << "version=" << kMetadataCacheVersion << '\n';
    emitField("title", entry.metadata.title);
    emitField("artist", entry.metadata.artist);
    emitField("album", entry.metadata.album);
    emitField("genre", entry.metadata.genre);
    emitField("composer", entry.metadata.composer);
    if (entry.metadata.duration_seconds) {
        output << "duration=" << *entry.metadata.duration_seconds << '\n';
    }
    if (!entry.release_mbid.empty()) {
        output << "release_mbid=" << entry.release_mbid << '\n';
    }
    if (!entry.release_group_mbid.empty()) {
        output << "release_group_mbid=" << entry.release_group_mbid << '\n';
    }
    if (entry.identity.found) {
        output << "identity_found=1\n";
        output << "identity_confidence=" << entry.identity.confidence << '\n';
        output << "identity_source=" << entry.identity.source << '\n';
        output << "identity_fingerprint_verified=" << (entry.identity.audio_fingerprint_verified ? "1" : "0") << '\n';
        if (!entry.identity.recording_mbid.empty()) {
            output << "recording_mbid=" << entry.identity.recording_mbid << '\n';
        }
        if (!entry.identity.release_mbid.empty()) {
            output << "identity_release_mbid=" << entry.identity.release_mbid << '\n';
        }
        if (!entry.identity.release_group_mbid.empty()) {
            output << "identity_release_group_mbid=" << entry.identity.release_group_mbid << '\n';
        }
        if (!entry.identity.fingerprint.empty()) {
            output << "identity_fingerprint=" << entry.identity.fingerprint << '\n';
        }
    }
    output << "online_metadata_attempted=" << (entry.online_metadata_attempted ? "1" : "0") << '\n';
    output << "online_artwork_attempted=" << (entry.online_artwork_attempted ? "1" : "0") << '\n';
    output << "online_lyrics_attempted=" << (entry.online_lyrics_attempted ? "1" : "0") << '\n';
    output << "track_identity_attempted=" << (entry.track_identity_attempted ? "1" : "0") << '\n';
    output << "track_identity_version=" << entry.track_identity_version << '\n';
    output << "lyrics_lookup_version=" << entry.lyrics_lookup_version << '\n';
}

CacheEntry loadMetadataCache(const SharedRuntimeCache& cache, std::string_view key)
{
    const std::lock_guard lock{cache.file_mutex};
    CacheEntry entry{};
    std::ifstream input(metadataCachePath(cache, key), std::ios::binary);
    if (!input.is_open()) {
        return entry;
    }

    std::string line{};
    int version = 0;
    while (std::getline(input, line)) {
        const auto pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        const auto key_name = line.substr(0, pos);
        const auto value = line.substr(pos + 1);
        if (key_name == "version") {
            try {
                version = std::stoi(value);
            } catch (...) {
                version = 0;
            }
        } else if (key_name == "title") {
            entry.metadata.title = repairMetadataText(value);
        } else if (key_name == "artist") {
            entry.metadata.artist = repairMetadataText(value);
        } else if (key_name == "album") {
            entry.metadata.album = repairMetadataText(value);
        } else if (key_name == "genre") {
            entry.metadata.genre = repairMetadataText(value);
        } else if (key_name == "composer") {
            entry.metadata.composer = repairMetadataText(value);
        } else if (key_name == "duration") {
            try {
                entry.metadata.duration_seconds = std::stoi(value);
            } catch (...) {
            }
        } else if (key_name == "release_mbid") {
            entry.release_mbid = value;
        } else if (key_name == "release_group_mbid") {
            entry.release_group_mbid = value;
        } else if (key_name == "identity_found") {
            entry.identity.found = value == "1";
        } else if (key_name == "identity_confidence") {
            try {
                entry.identity.confidence = std::stod(value);
            } catch (...) {
                entry.identity.confidence = 0.0;
            }
        } else if (key_name == "identity_source") {
            entry.identity.source = value;
        } else if (key_name == "identity_fingerprint_verified") {
            entry.identity.audio_fingerprint_verified = value == "1";
        } else if (key_name == "recording_mbid") {
            entry.identity.recording_mbid = value;
        } else if (key_name == "identity_release_mbid") {
            entry.identity.release_mbid = value;
        } else if (key_name == "identity_release_group_mbid") {
            entry.identity.release_group_mbid = value;
        } else if (key_name == "identity_fingerprint") {
            entry.identity.fingerprint = value;
        } else if (key_name == "online_metadata_attempted") {
            entry.online_metadata_attempted = value == "1";
        } else if (key_name == "online_artwork_attempted") {
            entry.online_artwork_attempted = value == "1";
        } else if (key_name == "online_lyrics_attempted") {
            entry.online_lyrics_attempted = value == "1";
        } else if (key_name == "track_identity_attempted") {
            entry.track_identity_attempted = value == "1";
        } else if (key_name == "track_identity_version") {
            try {
                entry.track_identity_version = std::stoi(value);
            } catch (...) {
                entry.track_identity_version = 0;
            }
        } else if (key_name == "lyrics_lookup_version") {
            try {
                entry.lyrics_lookup_version = std::stoi(value);
            } catch (...) {
                entry.lyrics_lookup_version = 0;
            }
        }
    }

    if (version != kMetadataCacheVersion) {
        entry.metadata = {};
        entry.online_metadata_attempted = false;
        entry.online_artwork_attempted = false;
        entry.online_lyrics_attempted = false;
        entry.track_identity_attempted = false;
        entry.track_identity_version = 0;
        entry.lyrics_lookup_version = 0;
        entry.identity = {};
        entry.release_mbid.clear();
        entry.release_group_mbid.clear();
    }

    if (entry.identity.found) {
        entry.identity.metadata = entry.metadata;
        if (entry.identity.release_mbid.empty()) {
            entry.identity.release_mbid = entry.release_mbid;
        }
        if (entry.identity.release_group_mbid.empty()) {
            entry.identity.release_group_mbid = entry.release_group_mbid;
        }
    }

    return entry;
}

void storeLyricsCache(const SharedRuntimeCache& cache, std::string_view key, const app::TrackLyrics& lyrics)
{
    const std::lock_guard lock{cache.file_mutex};
    std::ofstream output(lyricsCachePath(cache, key), std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        return;
    }
    const auto escape = [](std::string_view value) {
        std::string result{};
        for (const char ch : value) {
            switch (ch) {
            case '\\':
                result += "\\\\";
                break;
            case '\n':
                result += "\\n";
                break;
            case '\r':
                result += "\\r";
                break;
            case '\t':
                result += "\\t";
                break;
            default:
                result.push_back(ch);
                break;
            }
        }
        return result;
    };
    if (lyrics.plain) {
        output << "plain=" << escape(*lyrics.plain) << '\n';
    }
    if (lyrics.synced) {
        output << "synced=" << escape(*lyrics.synced) << '\n';
    }
    output << "source=" << escape(lyrics.source) << '\n';
}

app::TrackLyrics loadLyricsCache(const SharedRuntimeCache& cache, std::string_view key)
{
    const std::lock_guard lock{cache.file_mutex};
    app::TrackLyrics lyrics{};
    std::ifstream input(lyricsCachePath(cache, key), std::ios::binary);
    if (!input.is_open()) {
        return lyrics;
    }
    std::string line{};
    while (std::getline(input, line)) {
        const auto pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        const auto key_name = line.substr(0, pos);
        const auto unescape = [](std::string_view value) {
            std::string result{};
            bool escape = false;
            for (const char ch : value) {
                if (escape) {
                    switch (ch) {
                    case 'n':
                        result.push_back('\n');
                        break;
                    case 'r':
                        result.push_back('\r');
                        break;
                    case 't':
                        result.push_back('\t');
                        break;
                    case '\\':
                        result.push_back('\\');
                        break;
                    default:
                        result.push_back(ch);
                        break;
                    }
                    escape = false;
                } else if (ch == '\\') {
                    escape = true;
                } else {
                    result.push_back(ch);
                }
            }
            if (escape) {
                result.push_back('\\');
            }
            return result;
        };
        const auto value = unescape(line.substr(pos + 1));
        if (key_name == "plain") {
            lyrics.plain = value;
        } else if (key_name == "synced") {
            lyrics.synced = value;
        } else if (key_name == "source") {
            lyrics.source = value;
        }
    }
    return lyrics;
}


} // namespace lofibox::platform::host::runtime_detail
