// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/runtime_enrichment_clients.h"
#include "platform/host/runtime_enrichment_client_helpers.h"
#include "platform/host/runtime_logger.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace lofibox::platform::host::runtime_detail {
LrclibClient::LrclibClient()
{
#if defined(_WIN32)
    curl_path_ = resolveExecutablePath(L"CURL_PATH", L"curl.exe");
#elif defined(__linux__)
    curl_path_ = resolveExecutablePath("CURL_PATH", "curl");
#endif
}

bool LrclibClient::available() const
{
    return curl_path_.has_value() || resolvePythonPath().has_value();
}

app::TrackLyrics LrclibClient::fetch(const fs::path& path, const app::TrackMetadata& metadata) const
{
    app::TrackLyrics lyrics{};
    if (!available()) {
        return lyrics;
    }

    const auto seeds = lrclibLookupSeedsFromMetadata(path, metadata);
    if (seeds.empty()) {
        logRuntime(RuntimeLogLevel::Debug, "lyrics", "LRCLIB skipped: no usable title for " + pathUtf8String(path));
        return lyrics;
    }

    std::vector<std::string> attempted_urls{};
    auto fetch_url_once = [&](const std::string& url) -> std::optional<std::string> {
        if (std::find(attempted_urls.begin(), attempted_urls.end(), url) != attempted_urls.end()) {
            return std::nullopt;
        }
        attempted_urls.push_back(url);
        return captureUrl(curl_path_, url);
    };

    int get_attempts = 0;
    for (const auto& seed : seeds) {
        if (get_attempts >= 1) {
            break;
        }
        std::string url = "https://lrclib.net/api/get?track_name=" + urlEncode(seed.title);
        if (!seed.artist.empty()) {
            url += "&artist_name=" + urlEncode(seed.artist);
        }
        if (!seed.album.empty()) {
            url += "&album_name=" + urlEncode(seed.album);
        }
        if (seed.duration_seconds) {
            url += "&duration=" + std::to_string(*seed.duration_seconds);
        }
        logRuntime(RuntimeLogLevel::Debug, "lyrics", "LRCLIB get query for " + pathUtf8String(path) + ": " + seed.artist + " - " + seed.title);
        ++get_attempts;
        if (const auto json = fetch_url_once(url)) {
            lyrics = bestLrclibLyricsFromJson(*json, seeds);
            if (lyrics.plain || lyrics.synced) {
                logRuntime(RuntimeLogLevel::Info, "lyrics", "LRCLIB get hit for " + pathUtf8String(path));
                return lyrics;
            }
        }
    }

    for (const auto& seed : seeds) {
        std::string url = "https://lrclib.net/api/search?track_name=" + urlEncode(seed.title);
        if (!seed.artist.empty()) {
            url += "&artist_name=" + urlEncode(seed.artist);
        }
        if (!seed.album.empty()) {
            url += "&album_name=" + urlEncode(seed.album);
        }
        logRuntime(RuntimeLogLevel::Debug, "lyrics", "LRCLIB search query for " + pathUtf8String(path) + ": " + seed.artist + " - " + seed.title);
        if (const auto json = fetch_url_once(url)) {
            lyrics = bestLrclibLyricsFromJson(*json, seeds);
            if (lyrics.plain || lyrics.synced) {
                logRuntime(RuntimeLogLevel::Info, "lyrics", "LRCLIB search hit for " + pathUtf8String(path));
                return lyrics;
            }
        }
    }

    for (const auto& seed : seeds) {
        const std::string query = trim(seed.artist.empty() ? seed.title : seed.artist + " " + seed.title);
        if (query.empty()) {
            continue;
        }
        const std::string url = "https://lrclib.net/api/search?q=" + urlEncode(query);
        logRuntime(RuntimeLogLevel::Debug, "lyrics", "LRCLIB q search for " + pathUtf8String(path) + ": " + query);
        if (const auto json = fetch_url_once(url)) {
            lyrics = bestLrclibLyricsFromJson(*json, seeds);
            if (lyrics.plain || lyrics.synced) {
                logRuntime(RuntimeLogLevel::Info, "lyrics", "LRCLIB q search hit for " + pathUtf8String(path));
                return lyrics;
            }
        }
    }

    return lyrics;
}

LyricsOvhClient::LyricsOvhClient()
{
#if defined(_WIN32)
    curl_path_ = resolveExecutablePath(L"CURL_PATH", L"curl.exe");
#elif defined(__linux__)
    curl_path_ = resolveExecutablePath("CURL_PATH", "curl");
#endif
}

bool LyricsOvhClient::available() const
{
    return curl_path_.has_value() || resolvePythonPath().has_value();
}

app::TrackLyrics LyricsOvhClient::fetch(const fs::path& path, const app::TrackMetadata& metadata) const
{
    app::TrackLyrics lyrics{};
    if (!available()) {
        return lyrics;
    }

    const auto seeds = lrclibLookupSeedsFromMetadata(path, metadata);
    if (seeds.empty()) {
        logRuntime(RuntimeLogLevel::Debug, "lyrics", "lyrics.ovh skipped: no usable title for " + pathUtf8String(path));
        return lyrics;
    }

    std::vector<std::string> attempted_urls{};
    auto fetch_url_once = [&](const std::string& url) -> std::optional<std::string> {
        if (std::find(attempted_urls.begin(), attempted_urls.end(), url) != attempted_urls.end()) {
            return std::nullopt;
        }
        attempted_urls.push_back(url);
        return captureUrl(curl_path_, url);
    };

    auto fetch_direct = [&](const std::string& artist, const std::string& title) -> app::TrackLyrics {
        if (artist.empty() || title.empty()) {
            return {};
        }
        const std::string url = "https://api.lyrics.ovh/v1/" + urlEncode(artist) + "/" + urlEncode(title);
        logRuntime(RuntimeLogLevel::Debug, "lyrics", "lyrics.ovh direct query for " + pathUtf8String(path) + ": " + artist + " - " + title);
        if (const auto json = fetch_url_once(url)) {
            return lyricsOvhLyricsFromJson(*json);
        }
        return {};
    };

    for (const auto& seed : seeds) {
        if (seed.weak_title_only || seed.artist.empty()) {
            continue;
        }
        lyrics = fetch_direct(seed.artist, seed.title);
        if (lyrics.plain || lyrics.synced) {
            logRuntime(RuntimeLogLevel::Info, "lyrics", "lyrics.ovh direct hit for " + pathUtf8String(path));
            return lyrics;
        }
    }

    for (const auto& seed : seeds) {
        if (seed.weak_title_only || seed.artist.empty()) {
            continue;
        }
        const std::string query = trim(seed.artist.empty() ? seed.title : seed.artist + " " + seed.title);
        if (query.empty()) {
            continue;
        }
        const std::string url = "https://api.lyrics.ovh/suggest/" + urlEncode(query);
        logRuntime(RuntimeLogLevel::Debug, "lyrics", "lyrics.ovh suggest query for " + pathUtf8String(path) + ": " + query);
        if (const auto json = fetch_url_once(url)) {
            for (const auto& suggestion : lyricsOvhSuggestionsFromJson(*json, seeds)) {
                lyrics = fetch_direct(suggestion.artist, suggestion.title);
                if (lyrics.plain || lyrics.synced) {
                    logRuntime(RuntimeLogLevel::Info, "lyrics", "lyrics.ovh suggestion hit for " + pathUtf8String(path));
                    return lyrics;
                }
            }
        }
    }

    return lyrics;
}

} // namespace lofibox::platform::host::runtime_detail

