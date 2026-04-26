// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "app/runtime_services.h"
#include "platform/host/runtime_enrichment_clients.h"

namespace lofibox::platform::host::runtime_detail {
namespace fs = std::filesystem;

inline constexpr std::string_view kDefaultAcoustIdClientKey = "mWsj28v7ez";

struct LrclibLookupSeed {
    std::string title{};
    std::string artist{};
    std::string album{};
    std::optional<int> duration_seconds{};
    bool weak_title_only{false};
};

struct LyricsOvhSuggestion {
    std::string title{};
    std::string artist{};
};

std::vector<FilenameMetadataGuess> musicBrainzGuessesFromSeed(const fs::path& path, const app::TrackMetadata& seed_metadata);
std::vector<std::string> musicBrainzQueryCandidates(const std::vector<FilenameMetadataGuess>& guesses);
MusicBrainzLookupResult parseBestMusicBrainzRecording(std::string_view json, const std::vector<FilenameMetadataGuess>& guesses);
app::TrackIdentity parseBestAcoustIdIdentity(std::string_view json, const app::TrackMetadata& seed_metadata);
std::vector<LrclibLookupSeed> lrclibLookupSeedsFromMetadata(const fs::path& path, const app::TrackMetadata& metadata);
app::TrackLyrics bestLrclibLyricsFromJson(std::string_view json, const std::vector<LrclibLookupSeed>& seeds);
app::TrackLyrics lyricsOvhLyricsFromJson(std::string_view json);
std::vector<LyricsOvhSuggestion> lyricsOvhSuggestionsFromJson(std::string_view json, const std::vector<LrclibLookupSeed>& seeds);
std::optional<std::string> captureUrl(const std::optional<fs::path>& curl_path, const std::string& url);
std::optional<std::string> captureFormPost(
    const std::optional<fs::path>& curl_path,
    const std::string& url,
    const std::vector<std::pair<std::string, std::string>>& fields);
bool downloadUrlToPngFile(
    const std::optional<fs::path>& curl_path,
    const FfmpegArtworkExtractor& extractor,
    const std::string& url,
    const fs::path& output_path);

} // namespace lofibox::platform::host::runtime_detail
