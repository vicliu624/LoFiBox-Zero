// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/runtime_enrichment_clients.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <optional>
#include <cstdlib>
#include <cmath>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "platform/host/png_canvas_loader.h"
#include "platform/host/runtime_logger.h"

namespace fs = std::filesystem;

namespace lofibox::platform::host::runtime_detail {
namespace {

constexpr std::string_view kDefaultAcoustIdClientKey = "mWsj28v7ez";

std::string stripParentheticalNoise(std::string value)
{
    std::string stripped{};
    int depth = 0;
    for (const char ch : value) {
        if (ch == '(' || ch == '[' || ch == '{') {
            ++depth;
            continue;
        }
        if ((ch == ')' || ch == ']' || ch == '}') && depth > 0) {
            --depth;
            continue;
        }
        if (depth == 0) {
            stripped.push_back(ch);
        }
    }
    return trim(stripped);
}

std::string stripTrailingVersionNoise(std::string value)
{
    value = trim(std::move(value));
    const std::vector<std::string> suffixes{
        " single version",
        " album version",
        " radio edit",
        " remastered",
        " remaster"};
    std::string lowered = asciiLower(value);
    for (const auto& suffix : suffixes) {
        if (lowered.size() > suffix.size() && lowered.ends_with(suffix)) {
            value = trim(value.substr(0, value.size() - suffix.size()));
            lowered = asciiLower(value);
            break;
        }
    }
    return value;
}

std::vector<std::string_view> extractArrayObjectBlocks(std::string_view json, std::string_view marker)
{
    std::vector<std::string_view> blocks{};
    const auto marker_pos = json.find(marker);
    if (marker_pos == std::string_view::npos) {
        return blocks;
    }
    const auto array_begin = json.find('[', marker_pos);
    if (array_begin == std::string_view::npos) {
        return blocks;
    }

    bool in_string = false;
    bool escape = false;
    int depth = 0;
    std::size_t block_begin = std::string_view::npos;
    for (std::size_t index = array_begin + 1; index < json.size(); ++index) {
        const char ch = json[index];
        if (escape) {
            escape = false;
            continue;
        }
        if (ch == '\\' && in_string) {
            escape = true;
            continue;
        }
        if (ch == '"') {
            in_string = !in_string;
            continue;
        }
        if (in_string) {
            continue;
        }
        if (ch == '{') {
            if (depth == 0) {
                block_begin = index;
            }
            ++depth;
        } else if (ch == '}') {
            --depth;
            if (depth == 0 && block_begin != std::string_view::npos) {
                blocks.push_back(json.substr(block_begin, index - block_begin + 1));
                block_begin = std::string_view::npos;
            }
        } else if (ch == ']' && depth == 0) {
            break;
        }
    }
    return blocks;
}

std::optional<double> parseJsonDouble(std::string_view json, std::string_view marker)
{
    const auto pos = json.find(marker);
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }
    std::size_t cursor = pos + marker.size();
    while (cursor < json.size() && std::isspace(static_cast<unsigned char>(json[cursor])) != 0) {
        ++cursor;
    }
    std::size_t end = cursor;
    while (end < json.size()
        && (std::isdigit(static_cast<unsigned char>(json[end])) != 0 || json[end] == '-' || json[end] == '.')) {
        ++end;
    }
    if (end == cursor) {
        return std::nullopt;
    }
    try {
        return std::stod(std::string(json.substr(cursor, end - cursor)));
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<std::string> extractJsonStringField(std::string_view json, std::string_view key, std::size_t start = 0)
{
    const std::string quoted_key = "\"" + std::string(key) + "\"";
    const auto key_pos = json.find(quoted_key, start);
    if (key_pos == std::string_view::npos) {
        return std::nullopt;
    }
    const auto colon_pos = json.find(':', key_pos + quoted_key.size());
    if (colon_pos == std::string_view::npos) {
        return std::nullopt;
    }
    std::size_t cursor = colon_pos + 1;
    while (cursor < json.size() && std::isspace(static_cast<unsigned char>(json[cursor])) != 0) {
        ++cursor;
    }
    if (cursor >= json.size() || json[cursor] != '"') {
        return std::nullopt;
    }

    return parseJsonStringAt(json, cursor);
}

std::string musicBrainzStructuredQuery(const std::string& title, const std::string& artist)
{
    std::string query = "recording:\"" + title + "\"";
    if (!artist.empty() && asciiLower(artist) != "unknown") {
        query += " AND artist:\"" + artist + "\"";
    }
    return query;
}

std::string normalizedMatchText(std::string value)
{
    value = stripParentheticalNoise(std::move(value));
    value = replaceAll(std::move(value), "_", " ");
    value = replaceAll(std::move(value), "-", " ");
    std::string normalized{};
    bool previous_space = false;
    for (const unsigned char ch : value) {
        if (ch < 0x80U && std::isalnum(ch) == 0) {
            if (!previous_space) {
                normalized.push_back(' ');
                previous_space = true;
            }
            continue;
        }
        if (ch < 0x80U) {
            normalized.push_back(static_cast<char>(std::tolower(ch)));
        } else {
            normalized.push_back(static_cast<char>(ch));
        }
        previous_space = false;
    }
    return trim(normalized);
}

std::vector<std::string> musicBrainzQueryCandidates(const std::vector<FilenameMetadataGuess>& guesses)
{
    std::vector<std::string> queries{};
    for (const auto& guess : guesses) {
        const auto& title = guess.title;
        const auto& artist = guess.artist;
        if (title.empty()) {
            continue;
        }

        queries.push_back(musicBrainzStructuredQuery(title, artist));

        const auto stripped_title = stripParentheticalNoise(title);
        if (!stripped_title.empty() && stripped_title != title) {
            queries.push_back(musicBrainzStructuredQuery(stripped_title, artist));
        }

        if (!artist.empty() && asciiLower(artist) != "unknown") {
            queries.push_back("\"" + title + "\" AND \"" + artist + "\"");
            if (!stripped_title.empty() && stripped_title != title) {
                queries.push_back("\"" + stripped_title + "\" AND \"" + artist + "\"");
            }
        }
        queries.push_back(musicBrainzStructuredQuery(title, {}));
        if (!stripped_title.empty() && stripped_title != title) {
            queries.push_back(musicBrainzStructuredQuery(stripped_title, {}));
        }
    }

    std::vector<std::string> unique_queries{};
    for (const auto& query : queries) {
        if (std::find(unique_queries.begin(), unique_queries.end(), query) == unique_queries.end()) {
            unique_queries.push_back(query);
        }
    }
    return unique_queries;
}

std::vector<FilenameMetadataGuess> musicBrainzGuessesFromSeed(
    const fs::path& path,
    const app::TrackMetadata& seed_metadata)
{
    std::vector<FilenameMetadataGuess> guesses{};
    if (seed_metadata.title
        && !seed_metadata.title->empty()
        && metadataCompatibleWithFilename(path, seed_metadata)) {
        guesses.push_back(FilenameMetadataGuess{
            *seed_metadata.title,
            seed_metadata.artist.value_or("")});
    }
    const auto fallback_guesses = fallbackMetadataGuessesFromPath(path);
    for (const auto& guess : fallback_guesses) {
        const auto duplicate = std::find_if(
            guesses.begin(),
            guesses.end(),
            [&](const FilenameMetadataGuess& existing) {
                return existing.title == guess.title && existing.artist == guess.artist;
            });
        if (duplicate == guesses.end()) {
            guesses.push_back(guess);
        }
    }
    return guesses;
}

std::optional<int> extractMusicBrainzScore(std::string_view json)
{
    if (const auto score = parseJsonInt(json, "\"score\":")) {
        return score;
    }
    if (const auto score = extractJsonString(json, "\"score\":\"")) {
        try {
            return std::stoi(*score);
        } catch (...) {
        }
    }
    return std::nullopt;
}

std::optional<std::string> extractMusicBrainzReleaseGroupId(std::string_view json, std::size_t releases_pos)
{
    const auto release_group_pos = json.find("\"release-group\":{", releases_pos);
    if (release_group_pos == std::string_view::npos) {
        return std::nullopt;
    }
    return extractJsonString(json, "\"id\":\"", release_group_pos);
}

std::vector<std::string_view> extractRecordingBlocks(std::string_view json)
{
    std::vector<std::string_view> blocks{};
    const auto recordings_pos = findJsonBlock(json, "\"recordings\"");
    if (recordings_pos == std::string_view::npos) {
        return blocks;
    }

    const auto array_begin = json.find('[', recordings_pos);
    if (array_begin == std::string_view::npos) {
        return blocks;
    }

    bool in_string = false;
    bool escape = false;
    int depth = 0;
    std::size_t block_begin = std::string_view::npos;
    for (std::size_t index = array_begin + 1; index < json.size(); ++index) {
        const char ch = json[index];
        if (escape) {
            escape = false;
            continue;
        }
        if (ch == '\\' && in_string) {
            escape = true;
            continue;
        }
        if (ch == '"') {
            in_string = !in_string;
            continue;
        }
        if (in_string) {
            continue;
        }
        if (ch == '{') {
            if (depth == 0) {
                block_begin = index;
            }
            ++depth;
        } else if (ch == '}') {
            --depth;
            if (depth == 0 && block_begin != std::string_view::npos) {
                blocks.push_back(json.substr(block_begin, index - block_begin + 1));
                block_begin = std::string_view::npos;
            }
        } else if (ch == ']' && depth == 0) {
            break;
        }
    }
    return blocks;
}

MusicBrainzLookupResult parseMusicBrainzRecordingBlock(std::string_view json)
{
    MusicBrainzLookupResult result{};
    result.recording_mbid = extractJsonString(json, "\"id\":\"").value_or("");
    result.metadata.title = extractJsonString(json, "\"title\":\"");
    result.metadata.artist = extractJsonString(json, "\"name\":\"");
    const auto releases_pos = json.find("\"releases\":[");
    if (releases_pos != std::string::npos) {
        result.metadata.album = extractJsonString(json, "\"title\":\"", releases_pos);
        if (const auto release_id = extractJsonString(json, "\"id\":\"", releases_pos)) {
            result.release_mbid = *release_id;
        }
        if (const auto release_group_id = extractMusicBrainzReleaseGroupId(json, releases_pos)) {
            result.release_group_mbid = *release_group_id;
        }
    }
    result.found = result.metadata.title.has_value()
        || result.metadata.artist.has_value()
        || result.metadata.album.has_value()
        || !result.recording_mbid.empty()
        || !result.release_mbid.empty()
        || !result.release_group_mbid.empty();
    return result;
}

int scoreMusicBrainzCandidate(const MusicBrainzLookupResult& candidate, const std::vector<FilenameMetadataGuess>& guesses, int musicbrainz_score)
{
    int score = musicbrainz_score;
    const std::string candidate_title = normalizedMatchText(candidate.metadata.title.value_or(""));
    const std::string candidate_artist = normalizedMatchText(candidate.metadata.artist.value_or(""));

    for (const auto& guess : guesses) {
        const std::string guess_title = normalizedMatchText(guess.title);
        const std::string guess_artist = normalizedMatchText(guess.artist);
        if (!guess_title.empty() && candidate_title == guess_title) {
            score += 60;
        } else if (!guess_title.empty() && (candidate_title.find(guess_title) != std::string::npos || guess_title.find(candidate_title) != std::string::npos)) {
            score += 25;
        }
        if (!guess_artist.empty() && candidate_artist == guess_artist) {
            score += 45;
        } else if (!guess_artist.empty() && (candidate_artist.find(guess_artist) != std::string::npos || guess_artist.find(candidate_artist) != std::string::npos)) {
            score += 15;
        }
    }

    if (!candidate.release_group_mbid.empty()) {
        score += 8;
    }
    if (!candidate.release_mbid.empty()) {
        score += 5;
    }
    return score;
}

bool musicBrainzArtistMatches(const std::string& candidate_artist, const std::string& guess_artist)
{
    if (candidate_artist.empty() || guess_artist.empty()) {
        return false;
    }
    return candidate_artist == guess_artist
        || candidate_artist.find(guess_artist) != std::string::npos
        || guess_artist.find(candidate_artist) != std::string::npos;
}

bool isAcceptableMusicBrainzCandidate(const MusicBrainzLookupResult& candidate, const std::vector<FilenameMetadataGuess>& guesses)
{
    const std::string candidate_title = normalizedMatchText(candidate.metadata.title.value_or(""));
    const std::string candidate_artist = normalizedMatchText(candidate.metadata.artist.value_or(""));
    if (candidate_title.empty()) {
        return false;
    }

    for (const auto& guess : guesses) {
        const std::string guess_title = normalizedMatchText(guess.title);
        const std::string guess_artist = normalizedMatchText(guess.artist);
        if (guess_title.empty()) {
            continue;
        }
        if (candidate_title == guess_title) {
            return true;
        }
        const bool fuzzy_title_match =
            candidate_title.find(guess_title) != std::string::npos
            || guess_title.find(candidate_title) != std::string::npos;
        if (fuzzy_title_match && musicBrainzArtistMatches(candidate_artist, guess_artist)) {
            return true;
        }
    }

    return false;
}

MusicBrainzLookupResult parseBestMusicBrainzRecording(std::string_view json, const std::vector<FilenameMetadataGuess>& guesses)
{
    MusicBrainzLookupResult best{};
    int best_score = -1;
    for (const auto block : extractRecordingBlocks(json)) {
        auto candidate = parseMusicBrainzRecordingBlock(block);
        if (!candidate.found || !isAcceptableMusicBrainzCandidate(candidate, guesses)) {
            continue;
        }
        const int musicbrainz_score = extractMusicBrainzScore(block).value_or(0);
        const int score = scoreMusicBrainzCandidate(candidate, guesses, musicbrainz_score);
        if (score > best_score) {
            best_score = score;
            candidate.confidence = std::clamp(static_cast<double>(score) / 200.0, 0.0, 1.0);
            best = std::move(candidate);
        }
    }
    return best;
}

bool acoustIdCandidateCompatibleWithSeed(const app::TrackIdentity& candidate, const app::TrackMetadata& seed_metadata)
{
    if (!seed_metadata.title || seed_metadata.title->empty()) {
        return true;
    }
    const auto candidate_title = normalizedMatchText(candidate.metadata.title.value_or(""));
    const auto seed_title = normalizedMatchText(*seed_metadata.title);
    const auto candidate_artist = normalizedMatchText(candidate.metadata.artist.value_or(""));
    const auto seed_artist = normalizedMatchText(seed_metadata.artist.value_or(""));
    if (candidate_title.empty() || seed_title.empty()) {
        return true;
    }
    const bool title_matches = candidate_title == seed_title
        || candidate_title.find(seed_title) != std::string::npos
        || seed_title.find(candidate_title) != std::string::npos;
    if (!title_matches) {
        return candidate.confidence >= 0.92;
    }
    if (seed_artist.empty() || candidate_artist.empty()) {
        return true;
    }
    return candidate_artist == seed_artist
        || candidate_artist.find(seed_artist) != std::string::npos
        || seed_artist.find(candidate_artist) != std::string::npos
        || candidate.confidence >= 0.90;
}

app::TrackIdentity parseAcoustIdRecording(std::string_view recording, double score)
{
    app::TrackIdentity identity{};
    identity.recording_mbid = extractJsonStringField(recording, "id").value_or("");
    identity.metadata.title = extractJsonStringField(recording, "title");
    const auto artist_blocks = extractArrayObjectBlocks(recording, "\"artists\"");
    if (!artist_blocks.empty()) {
        identity.metadata.artist = extractJsonStringField(artist_blocks.front(), "name");
    }
    const auto release_group_blocks = extractArrayObjectBlocks(recording, "\"releasegroups\"");
    if (!release_group_blocks.empty()) {
        identity.release_group_mbid = extractJsonStringField(release_group_blocks.front(), "id").value_or("");
        if (!identity.metadata.album) {
            identity.metadata.album = extractJsonStringField(release_group_blocks.front(), "title");
        }
    }
    const auto release_blocks = extractArrayObjectBlocks(recording, "\"releases\"");
    if (!release_blocks.empty()) {
        identity.release_mbid = extractJsonStringField(release_blocks.front(), "id").value_or("");
        if (!identity.metadata.album) {
            identity.metadata.album = extractJsonStringField(release_blocks.front(), "title");
        }
    }
    if (const auto duration = parseJsonDouble(recording, "\"duration\":")) {
        identity.metadata.duration_seconds = static_cast<int>(std::lround(*duration));
    }
    identity.confidence = score;
    identity.source = "ACOUSTID";
    identity.audio_fingerprint_verified = true;
    identity.found = identity.metadata.title.has_value()
        || identity.metadata.artist.has_value()
        || !identity.recording_mbid.empty();
    return identity;
}

app::TrackIdentity parseBestAcoustIdIdentity(std::string_view json, const app::TrackMetadata& seed_metadata)
{
    app::TrackIdentity best{};
    for (const auto result_block : extractArrayObjectBlocks(json, "\"results\"")) {
        const double score = parseJsonDouble(result_block, "\"score\":").value_or(0.0);
        if (score < 0.75) {
            continue;
        }
        for (const auto recording_block : extractArrayObjectBlocks(result_block, "\"recordings\"")) {
            auto candidate = parseAcoustIdRecording(recording_block, score);
            if (!candidate.found || !acoustIdCandidateCompatibleWithSeed(candidate, seed_metadata)) {
                continue;
            }
            if (!best.found || candidate.confidence > best.confidence) {
                best = std::move(candidate);
            }
        }
    }
    return best;
}

struct LrclibLookupSeed {
    std::string title{};
    std::string artist{};
    std::string album{};
    std::optional<int> duration_seconds{};
    bool weak_title_only{false};
};

struct LrclibCandidate {
    app::TrackLyrics lyrics{};
    std::string title{};
    std::string artist{};
    std::string album{};
    std::optional<int> duration_seconds{};
    bool found{false};
};

struct LyricsOvhSuggestion {
    std::string title{};
    std::string artist{};
};

std::vector<std::string_view> extractTopLevelObjectBlocks(std::string_view json)
{
    std::vector<std::string_view> blocks{};
    const auto array_begin = json.find('[');
    std::size_t cursor = array_begin == std::string_view::npos ? 0 : array_begin + 1;
    bool in_string = false;
    bool escape = false;
    int depth = 0;
    std::size_t block_begin = std::string_view::npos;
    for (; cursor < json.size(); ++cursor) {
        const char ch = json[cursor];
        if (escape) {
            escape = false;
            continue;
        }
        if (ch == '\\' && in_string) {
            escape = true;
            continue;
        }
        if (ch == '"') {
            in_string = !in_string;
            continue;
        }
        if (in_string) {
            continue;
        }
        if (ch == '{') {
            if (depth == 0) {
                block_begin = cursor;
            }
            ++depth;
        } else if (ch == '}') {
            --depth;
            if (depth == 0 && block_begin != std::string_view::npos) {
                blocks.push_back(json.substr(block_begin, cursor - block_begin + 1));
                block_begin = std::string_view::npos;
                if (array_begin == std::string_view::npos) {
                    break;
                }
            }
        } else if (ch == ']' && depth == 0) {
            break;
        }
    }
    return blocks;
}

std::optional<int> extractJsonRoundedSeconds(std::string_view json, std::string_view marker)
{
    const auto pos = json.find(marker);
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }
    std::size_t cursor = pos + marker.size();
    while (cursor < json.size() && std::isspace(static_cast<unsigned char>(json[cursor])) != 0) {
        ++cursor;
    }
    std::size_t end = cursor;
    while (end < json.size()
        && (std::isdigit(static_cast<unsigned char>(json[end])) != 0 || json[end] == '-' || json[end] == '.')) {
        ++end;
    }
    if (end == cursor) {
        return std::nullopt;
    }
    try {
        return static_cast<int>(std::lround(std::stod(std::string(json.substr(cursor, end - cursor)))));
    } catch (...) {
        return std::nullopt;
    }
}

LrclibCandidate parseLrclibCandidate(std::string_view json)
{
    LrclibCandidate candidate{};
    candidate.title = extractJsonString(json, "\"trackName\":\"").value_or(extractJsonString(json, "\"name\":\"").value_or(""));
    candidate.artist = extractJsonString(json, "\"artistName\":\"").value_or("");
    candidate.album = extractJsonString(json, "\"albumName\":\"").value_or("");
    candidate.duration_seconds = extractJsonRoundedSeconds(json, "\"duration\":");
    candidate.lyrics.plain = extractLyricsText(json, "\"plainLyrics\":\"");
    candidate.lyrics.synced = extractLyricsText(json, "\"syncedLyrics\":\"");
    if (candidate.lyrics.plain || candidate.lyrics.synced) {
        candidate.lyrics.source = "LRCLIB";
        candidate.found = true;
    }
    return candidate;
}

std::vector<LrclibLookupSeed> lrclibLookupSeedsFromMetadata(
    const fs::path& path,
    const app::TrackMetadata& metadata)
{
    std::vector<LrclibLookupSeed> seeds{};
    auto add_seed = [&](LrclibLookupSeed seed) {
        seed.title = trim(std::move(seed.title));
        seed.artist = trim(std::move(seed.artist));
        seed.album = trim(std::move(seed.album));
        seed.weak_title_only = seed.weak_title_only || seed.artist.empty();
        if (seed.title.empty() || asciiLower(seed.title) == "unknown") {
            return;
        }
        const auto duplicate = std::find_if(seeds.begin(), seeds.end(), [&](const LrclibLookupSeed& existing) {
            return asciiLower(existing.title) == asciiLower(seed.title)
                && asciiLower(existing.artist) == asciiLower(seed.artist)
                && asciiLower(existing.album) == asciiLower(seed.album);
        });
        if (duplicate == seeds.end()) {
            seeds.push_back(std::move(seed));
        }
    };
    auto add_title_only_seed = [&](const std::string& title, std::optional<int> duration_seconds) {
        const auto stripped_title = stripParentheticalNoise(title);
        add_seed(LrclibLookupSeed{title, {}, {}, duration_seconds, true});
        if (!stripped_title.empty() && normalizedMatchText(stripped_title) != normalizedMatchText(title)) {
            add_seed(LrclibLookupSeed{stripped_title, {}, {}, duration_seconds, true});
        }
        const auto version_stripped_title = stripTrailingVersionNoise(stripped_title.empty() ? title : stripped_title);
        if (!version_stripped_title.empty() && normalizedMatchText(version_stripped_title) != normalizedMatchText(title)) {
            add_seed(LrclibLookupSeed{version_stripped_title, {}, {}, duration_seconds, true});
        }
    };
    auto add_seed_with_title_variants = [&](const std::string& title, const std::string& artist, const std::string& album, std::optional<int> duration_seconds) {
        add_seed(LrclibLookupSeed{title, artist, album, duration_seconds, artist.empty()});
        const auto stripped_title = stripParentheticalNoise(title);
        if (!stripped_title.empty() && normalizedMatchText(stripped_title) != normalizedMatchText(title)) {
            add_seed(LrclibLookupSeed{stripped_title, artist, album, duration_seconds, artist.empty()});
        }
        const auto version_stripped_title = stripTrailingVersionNoise(stripped_title.empty() ? title : stripped_title);
        if (!version_stripped_title.empty() && normalizedMatchText(version_stripped_title) != normalizedMatchText(title)) {
            add_seed(LrclibLookupSeed{version_stripped_title, artist, album, duration_seconds, artist.empty()});
        }
    };

    if (metadata.title && metadataCompatibleWithFilename(path, metadata)) {
        add_seed_with_title_variants(
            *metadata.title,
            metadata.artist.value_or(""),
            metadata.album.value_or(""),
            metadata.duration_seconds);
        add_title_only_seed(*metadata.title, metadata.duration_seconds);
    }

    for (const auto& guess : fallbackMetadataGuessesFromPath(path)) {
        add_seed_with_title_variants(
            guess.title,
            guess.artist,
            fallbackAlbumFromPath(path),
            metadata.duration_seconds);
        add_title_only_seed(guess.title, metadata.duration_seconds);
    }

    return seeds;
}

bool lrclibArtistMatches(const std::string& candidate_artist, const std::string& seed_artist)
{
    const auto candidate = normalizedMatchText(candidate_artist);
    const auto seed = normalizedMatchText(seed_artist);
    if (candidate.empty() || seed.empty()) {
        return false;
    }
    return candidate == seed
        || candidate.find(seed) != std::string::npos
        || seed.find(candidate) != std::string::npos;
}

bool lrclibTitleMatches(const std::string& candidate_title, const std::string& seed_title)
{
    const auto candidate = normalizedMatchText(candidate_title);
    const auto seed = normalizedMatchText(seed_title);
    if (candidate.empty() || seed.empty()) {
        return false;
    }
    return candidate == seed
        || candidate.find(seed) != std::string::npos
        || seed.find(candidate) != std::string::npos;
}

bool lrclibTitleExactlyMatches(const std::string& candidate_title, const std::string& seed_title)
{
    const auto candidate = normalizedMatchText(candidate_title);
    const auto seed = normalizedMatchText(seed_title);
    return !candidate.empty() && candidate == seed;
}

bool lrclibDurationTightlyMatches(const LrclibCandidate& candidate, const LrclibLookupSeed& seed)
{
    if (!seed.duration_seconds || !candidate.duration_seconds) {
        return false;
    }
    return std::abs(*seed.duration_seconds - *candidate.duration_seconds) <= 2;
}

bool lrclibDurationLooselyMatches(const LrclibCandidate& candidate, const LrclibLookupSeed& seed)
{
    if (!seed.duration_seconds || !candidate.duration_seconds) {
        return false;
    }
    return std::abs(*seed.duration_seconds - *candidate.duration_seconds) <= 8;
}

int scoreLrclibCandidate(const LrclibCandidate& candidate, const std::vector<LrclibLookupSeed>& seeds)
{
    int score = 0;
    if (candidate.lyrics.synced && !candidate.lyrics.synced->empty()) {
        score += 20;
    }
    if (candidate.lyrics.plain && !candidate.lyrics.plain->empty()) {
        score += 10;
    }
    for (const auto& seed : seeds) {
        if (seed.weak_title_only && (!lrclibTitleExactlyMatches(candidate.title, seed.title) || !lrclibDurationTightlyMatches(candidate, seed))) {
            continue;
        }
        if (lrclibTitleExactlyMatches(candidate.title, seed.title)) {
            score += 80;
        } else if (lrclibTitleMatches(candidate.title, seed.title)) {
            score += 35;
        }
        if (!seed.artist.empty() && lrclibArtistMatches(candidate.artist, seed.artist)) {
            score += 45;
        }
        if (!seed.album.empty() && normalizedMatchText(candidate.album) == normalizedMatchText(seed.album)) {
            score += 12;
        }
        if (seed.duration_seconds && candidate.duration_seconds) {
            const int delta = std::abs(*seed.duration_seconds - *candidate.duration_seconds);
            if (delta <= 2) {
                score += 20;
            } else if (delta <= 8) {
                score += 8;
            } else if (delta >= 40) {
                score -= 20;
            }
        }
    }
    return score;
}

bool acceptableLrclibCandidate(const LrclibCandidate& candidate, const std::vector<LrclibLookupSeed>& seeds)
{
    if (!candidate.found) {
        return false;
    }
    for (const auto& seed : seeds) {
        if (seed.weak_title_only) {
            if (lrclibTitleExactlyMatches(candidate.title, seed.title) && lrclibDurationTightlyMatches(candidate, seed)) {
                return true;
            }
            continue;
        }
        if (lrclibTitleExactlyMatches(candidate.title, seed.title) && lrclibArtistMatches(candidate.artist, seed.artist)) {
            return true;
        }
        if (lrclibTitleMatches(candidate.title, seed.title)
            && lrclibArtistMatches(candidate.artist, seed.artist)
            && lrclibDurationLooselyMatches(candidate, seed)) {
            return true;
        }
    }
    return false;
}

app::TrackLyrics bestLrclibLyricsFromJson(std::string_view json, const std::vector<LrclibLookupSeed>& seeds)
{
    LrclibCandidate best{};
    int best_score = -1;
    for (const auto block : extractTopLevelObjectBlocks(json)) {
        auto candidate = parseLrclibCandidate(block);
        if (!acceptableLrclibCandidate(candidate, seeds)) {
            continue;
        }
        const int score = scoreLrclibCandidate(candidate, seeds);
        if (score > best_score) {
            best_score = score;
            best = std::move(candidate);
        }
    }
    return best.found ? best.lyrics : app::TrackLyrics{};
}

app::TrackLyrics lyricsOvhLyricsFromJson(std::string_view json)
{
    app::TrackLyrics lyrics{};
    lyrics.plain = extractLyricsText(json, "\"lyrics\":\"");
    if (lyrics.plain && !lyrics.plain->empty()) {
        lyrics.source = "lyrics.ovh";
    }
    return lyrics;
}

bool acceptableLyricsOvhSuggestion(const LyricsOvhSuggestion& suggestion, const std::vector<LrclibLookupSeed>& seeds)
{
    if (suggestion.title.empty() || suggestion.artist.empty()) {
        return false;
    }
    return std::any_of(seeds.begin(), seeds.end(), [&](const LrclibLookupSeed& seed) {
        if (seed.weak_title_only || seed.artist.empty()) {
            return false;
        }
        return lrclibTitleMatches(suggestion.title, seed.title)
            && lrclibArtistMatches(suggestion.artist, seed.artist);
    });
}

std::vector<LyricsOvhSuggestion> lyricsOvhSuggestionsFromJson(std::string_view json, const std::vector<LrclibLookupSeed>& seeds)
{
    std::vector<LyricsOvhSuggestion> suggestions{};
    for (const auto block : extractTopLevelObjectBlocks(json)) {
        LyricsOvhSuggestion suggestion{};
        suggestion.title = extractJsonString(block, "\"title_short\":\"").value_or(extractJsonString(block, "\"title\":\"").value_or(""));
        if (const auto artist_pos = block.find("\"artist\":{"); artist_pos != std::string_view::npos) {
            suggestion.artist = extractJsonString(block, "\"name\":\"", artist_pos).value_or("");
        }
        if (!acceptableLyricsOvhSuggestion(suggestion, seeds)) {
            continue;
        }
        const auto duplicate = std::find_if(suggestions.begin(), suggestions.end(), [&](const LyricsOvhSuggestion& existing) {
            return normalizedMatchText(existing.title) == normalizedMatchText(suggestion.title)
                && normalizedMatchText(existing.artist) == normalizedMatchText(suggestion.artist);
        });
        if (duplicate == suggestions.end()) {
            suggestions.push_back(std::move(suggestion));
        }
    }
    return suggestions;
}

std::optional<std::string> captureUrl(const std::optional<fs::path>& curl_path, const std::string& url)
{
    if (curl_path) {
        const std::vector<std::string> curl_args{
            "-fsSL",
            "-A",
            "LoFiBoxZero/0.1 (local development)",
            url};
        return captureProcessOutput(*curl_path, curl_args);
    }

    const auto python_path = resolvePythonPath();
    if (!python_path) {
        return std::nullopt;
    }

    static const std::string code =
        "import sys, urllib.request\n"
        "req = urllib.request.Request(sys.argv[1], headers={'User-Agent': 'LoFiBoxZero/0.1 (local development)'})\n"
        "with urllib.request.urlopen(req, timeout=15) as response:\n"
        "    sys.stdout.buffer.write(response.read())\n";
    return captureProcessOutput(*python_path, {"-c", code, url});
}

std::optional<std::string> captureFormPost(
    const std::optional<fs::path>& curl_path,
    const std::string& url,
    const std::vector<std::pair<std::string, std::string>>& fields)
{
    if (curl_path) {
        std::vector<std::string> curl_args{
            "-fsSL",
            "-A",
            "LoFiBoxZero/0.1 (local development)"};
        for (const auto& [name, value] : fields) {
            curl_args.push_back("--data-urlencode");
            curl_args.push_back(name + "=" + value);
        }
        curl_args.push_back(url);
        return captureProcessOutput(*curl_path, curl_args);
    }

    const auto python_path = resolvePythonPath();
    if (!python_path) {
        return std::nullopt;
    }

    std::string field_json = "[";
    for (std::size_t index = 0; index < fields.size(); ++index) {
        if (index > 0) {
            field_json += ",";
        }
        field_json += "[\"" + jsonEscape(fields[index].first) + "\",\"" + jsonEscape(fields[index].second) + "\"]";
    }
    field_json += "]";

    static const std::string code =
        "import json, sys, urllib.parse, urllib.request\n"
        "fields = json.loads(sys.argv[2])\n"
        "data = urllib.parse.urlencode(fields).encode('utf-8')\n"
        "req = urllib.request.Request(sys.argv[1], data=data, headers={'User-Agent':'LoFiBoxZero/0.1 (local development)'})\n"
        "with urllib.request.urlopen(req, timeout=20) as response:\n"
        "    sys.stdout.buffer.write(response.read())\n";
    return captureProcessOutput(*python_path, {"-c", code, url, field_json});
}

bool downloadUrlToFile(const std::optional<fs::path>& curl_path, const std::string& url, const fs::path& output_path)
{
    std::error_code ec{};
    fs::create_directories(output_path.parent_path(), ec);
    if (curl_path) {
        const std::vector<std::string> curl_args{
            "-fsSL",
            "-A",
            "LoFiBoxZero/0.1 (local development)",
            "-o",
            pathUtf8String(output_path),
            url};
        return runProcess(*curl_path, curl_args) && fs::exists(output_path, ec) && !ec;
    }

    const auto python_path = resolvePythonPath();
    if (!python_path) {
        return false;
    }

    static const std::string code =
        "import sys, urllib.request, pathlib\n"
        "req = urllib.request.Request(sys.argv[1], headers={'User-Agent': 'LoFiBoxZero/0.1 (local development)'})\n"
        "with urllib.request.urlopen(req, timeout=15) as response:\n"
        "    pathlib.Path(sys.argv[2]).write_bytes(response.read())\n";
    return runProcess(*python_path, {"-c", code, url, pathUtf8String(output_path)}) && fs::exists(output_path, ec) && !ec;
}

bool downloadUrlToPngFile(
    const std::optional<fs::path>& curl_path,
    const FfmpegArtworkExtractor& extractor,
    const std::string& url,
    const fs::path& output_path)
{
    if (!extractor.available()) {
        return false;
    }

    fs::path raw_path = output_path;
    raw_path += ".download";

    std::error_code ec{};
    fs::remove(raw_path, ec);
    fs::remove(output_path, ec);

    if (!downloadUrlToFile(curl_path, url, raw_path)) {
        fs::remove(raw_path, ec);
        return false;
    }

    const auto materialized = extractor.materialize(raw_path, output_path);
    fs::remove(raw_path, ec);
    if (!materialized) {
        fs::remove(output_path, ec);
        return false;
    }
    return true;
}

} // namespace

FfprobeMetadataReader::FfprobeMetadataReader()
{
#if defined(_WIN32)
    executable_path_ = resolveExecutablePath(L"FFPROBE_PATH", L"ffprobe.exe");
#elif defined(__linux__)
    executable_path_ = resolveExecutablePath("FFPROBE_PATH", "ffprobe");
#endif
}

bool FfprobeMetadataReader::available() const
{
    return executable_path_.has_value();
}

app::TrackMetadata FfprobeMetadataReader::read(const fs::path& path) const
{
    if (!executable_path_ || !fs::exists(path)) {
        return {};
    }

    const std::vector<std::string> args{
        "-v",
        "error",
        "-select_streams",
        "a:0",
        "-show_entries",
        "format=duration:format_tags=title,artist,album,genre,composer:stream_tags=title,artist,album,genre,composer",
        "-of",
        "default=noprint_wrappers=1:nokey=0",
        pathUtf8String(path)};

    if (const auto output = captureProcessOutput(*executable_path_, args)) {
        return parseMetadataOutput(*output);
    }
    return {};
}

ChromaprintFingerprintProvider::ChromaprintFingerprintProvider()
{
#if defined(_WIN32)
    executable_path_ = resolveExecutablePath(L"FPCALC_PATH", L"fpcalc.exe");
#elif defined(__linux__)
    executable_path_ = resolveExecutablePath("FPCALC_PATH", "fpcalc");
#endif
}

bool ChromaprintFingerprintProvider::available() const
{
    return executable_path_.has_value();
}

AudioFingerprintResult ChromaprintFingerprintProvider::fingerprint(const fs::path& path) const
{
    AudioFingerprintResult result{};
    if (!available() || !fs::exists(path)) {
        return result;
    }

    const auto output = captureProcessOutput(*executable_path_, {pathUtf8String(path)});
    if (!output) {
        return result;
    }

    std::size_t start = 0;
    while (start <= output->size()) {
        const auto end = output->find('\n', start);
        const auto line = output->substr(start, end == std::string::npos ? std::string::npos : end - start);
        const auto separator = line.find('=');
        if (separator != std::string::npos) {
            const auto key = line.substr(0, separator);
            const auto value = trim(line.substr(separator + 1));
            if (key == "DURATION") {
                try {
                    result.duration_seconds = static_cast<int>(std::lround(std::stod(value)));
                } catch (...) {
                    result.duration_seconds = 0;
                }
            } else if (key == "FINGERPRINT") {
                result.fingerprint = value;
            }
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }

    result.found = !result.fingerprint.empty() && result.duration_seconds > 0;
    return result;
}

AcoustIdIdentityClient::AcoustIdIdentityClient()
{
#if defined(_WIN32)
    curl_path_ = resolveExecutablePath(L"CURL_PATH", L"curl.exe");
#elif defined(__linux__)
    curl_path_ = resolveExecutablePath("CURL_PATH", "curl");
#endif
    if (const char* key = std::getenv("ACOUSTID_API_KEY"); key != nullptr && *key != '\0') {
        api_key_ = key;
    } else if (const char* key = std::getenv("ACOUSTID_CLIENT_KEY"); key != nullptr && *key != '\0') {
        api_key_ = key;
    } else {
        api_key_ = std::string(kDefaultAcoustIdClientKey);
    }
}

bool AcoustIdIdentityClient::available() const
{
    return !api_key_.empty()
        && fingerprint_provider_.available()
        && (curl_path_.has_value() || resolvePythonPath().has_value());
}

app::TrackIdentity AcoustIdIdentityClient::lookup(const fs::path& path, const app::TrackMetadata& seed_metadata) const
{
    app::TrackIdentity identity{};
    if (!available()) {
        if (api_key_.empty()) {
            logRuntime(RuntimeLogLevel::Debug, "identity", "AcoustID skipped: missing ACOUSTID_API_KEY/ACOUSTID_CLIENT_KEY");
        } else if (!fingerprint_provider_.available()) {
            logRuntime(RuntimeLogLevel::Debug, "identity", "AcoustID skipped: fpcalc unavailable");
        }
        return identity;
    }

    const auto fingerprint = fingerprint_provider_.fingerprint(path);
    if (!fingerprint.found) {
        logRuntime(RuntimeLogLevel::Warn, "identity", "Chromaprint fingerprint failed for " + pathUtf8String(path));
        return identity;
    }

    logRuntime(RuntimeLogLevel::Debug, "identity", "AcoustID lookup started for " + pathUtf8String(path));
    const std::vector<std::pair<std::string, std::string>> fields{
        {"client", api_key_},
        {"meta", "recordings releasegroups releases"},
        {"duration", std::to_string(fingerprint.duration_seconds)},
        {"fingerprint", fingerprint.fingerprint}};
    if (const auto json = captureFormPost(curl_path_, "https://api.acoustid.org/v2/lookup", fields)) {
        identity = parseBestAcoustIdIdentity(*json, seed_metadata);
        if (identity.found) {
            if (!identity.metadata.duration_seconds) {
                identity.metadata.duration_seconds = fingerprint.duration_seconds;
            }
            logRuntime(RuntimeLogLevel::Info, "identity", "AcoustID identity hit for " + pathUtf8String(path));
        } else {
            logRuntime(RuntimeLogLevel::Warn, "identity", "AcoustID identity miss for " + pathUtf8String(path));
        }
    } else {
        logRuntime(RuntimeLogLevel::Warn, "identity", "AcoustID request failed for " + pathUtf8String(path));
    }
    return identity;
}

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

FfmpegArtworkExtractor::FfmpegArtworkExtractor()
{
#if defined(_WIN32)
    executable_path_ = resolveExecutablePath(L"FFMPEG_PATH", L"ffmpeg.exe");
#elif defined(__linux__)
    executable_path_ = resolveExecutablePath("FFMPEG_PATH", "ffmpeg");
#endif
}

bool FfmpegArtworkExtractor::available() const
{
    return executable_path_.has_value();
}

std::optional<core::Canvas> FfmpegArtworkExtractor::extractEmbedded(const fs::path& path, const fs::path& cache_path) const
{
    if (!executable_path_ || !fs::exists(path)) {
        return std::nullopt;
    }
    std::error_code ec{};
    fs::create_directories(cache_path.parent_path(), ec);
    fs::remove(cache_path, ec);
    const std::vector<std::string> args{
        "-loglevel", "error",
        "-nostdin",
        "-i", pathUtf8String(path),
        "-map", "0:v:0?",
        "-frames:v", "1",
        "-c:v", "png",
        "-y", pathUtf8String(cache_path)};

    if (!runProcess(*executable_path_, args) || !fs::exists(cache_path, ec) || ec) {
        fs::remove(cache_path, ec);
        return std::nullopt;
    }

    return loadPngCanvas(cache_path);
}

std::optional<core::Canvas> FfmpegArtworkExtractor::materialize(const fs::path& candidate, const fs::path& cache_path) const
{
    if (!executable_path_) {
        return std::nullopt;
    }
    std::error_code ec{};
    fs::create_directories(cache_path.parent_path(), ec);
    fs::remove(cache_path, ec);
    const std::vector<std::string> args{
        "-loglevel", "error",
        "-nostdin",
        "-i", pathUtf8String(candidate),
        "-frames:v", "1",
        "-y", pathUtf8String(cache_path)};
    if (!runProcess(*executable_path_, args) || !fs::exists(cache_path, ec) || ec) {
        fs::remove(cache_path, ec);
        return std::nullopt;
    }
    return loadPngCanvas(cache_path);
}

CoverArtArchiveClient::CoverArtArchiveClient()
{
#if defined(_WIN32)
    curl_path_ = resolveExecutablePath(L"CURL_PATH", L"curl.exe");
#elif defined(__linux__)
    curl_path_ = resolveExecutablePath("CURL_PATH", "curl");
#endif
}

bool CoverArtArchiveClient::available() const
{
    return extractor_.available() && (curl_path_.has_value() || resolvePythonPath().has_value());
}

bool CoverArtArchiveClient::fetchFrontCover(std::string_view release_mbid, const fs::path& output_path) const
{
    if (!available() || release_mbid.empty()) {
        return false;
    }
    const std::string url = "https://coverartarchive.org/release/" + std::string(release_mbid) + "/front-250";
    return downloadUrlToPngFile(curl_path_, extractor_, url, output_path);
}

bool CoverArtArchiveClient::fetchReleaseGroupFrontCover(std::string_view release_group_mbid, const fs::path& output_path) const
{
    if (!available() || release_group_mbid.empty()) {
        return false;
    }
    const std::string url = "https://coverartarchive.org/release-group/" + std::string(release_group_mbid) + "/front-250";
    return downloadUrlToPngFile(curl_path_, extractor_, url, output_path);
}

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
