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
MusicBrainzMetadataClient::MusicBrainzMetadataClient()
{
#if defined(_WIN32)
    curl_path_ = resolveExecutablePath(L"CURL_PATH", L"curl.exe");
#elif defined(__linux__)
    curl_path_ = resolveExecutablePath("CURL_PATH", "curl");
#endif
}

bool MusicBrainzMetadataClient::available() const
{
    return curl_path_.has_value() || resolvePythonPath().has_value();
}

MusicBrainzLookupResult MusicBrainzMetadataClient::lookup(
    const fs::path& path,
    SharedRuntimeCache& cache,
    const app::TrackMetadata& seed_metadata) const
{
    MusicBrainzLookupResult result{};
    if (!available()) {
        return result;
    }

    respectMusicBrainzRateLimit(cache);

    const auto guesses = musicBrainzGuessesFromSeed(path, seed_metadata);
    const auto queries = musicBrainzQueryCandidates(guesses);
    for (std::size_t index = 0; index < queries.size(); ++index) {
        if (index > 0) {
            respectMusicBrainzRateLimit(cache);
        }
        const auto& query = queries[index];
        const std::string url =
            "https://musicbrainz.org/ws/2/recording/?query=" + urlEncode(query) + "&fmt=json&limit=5";
        logRuntime(RuntimeLogLevel::Debug,
                   "metadata",
                   "MusicBrainz query for " + pathUtf8String(path) + ": " + query);
        if (const auto json = captureUrl(curl_path_, url)) {
            result = parseBestMusicBrainzRecording(*json, guesses);
            if (result.found) {
                logRuntime(RuntimeLogLevel::Info,
                           "metadata",
                           "MusicBrainz metadata hit for " + pathUtf8String(path));
                return result;
            }
        } else {
            logRuntime(RuntimeLogLevel::Warn, "metadata", "MusicBrainz request failed for " + pathUtf8String(path));
        }
    }
    return result;
}

} // namespace lofibox::platform::host::runtime_detail

