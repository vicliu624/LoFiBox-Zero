// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/runtime_host_internal.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cwctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "platform/host/runtime_logger.h"

namespace fs = std::filesystem;

namespace lofibox::platform::host::runtime_detail {

std::string trim(std::string value)
{
    while (!value.empty() && (value.back() == '\n' || value.back() == '\r' || value.back() == ' ' || value.back() == '\t')) {
        value.pop_back();
    }
    std::size_t offset = 0;
    while (offset < value.size() && (value[offset] == ' ' || value[offset] == '\t')) {
        ++offset;
    }
    return value.substr(offset);
}

std::string asciiLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::optional<std::string> nonEmptyValue(std::string value)
{
    value = trim(std::move(value));
    if (value.empty()) {
        return std::nullopt;
    }
    return value;
}

std::string replaceAll(std::string value, std::string_view from, std::string_view to)
{
    std::size_t pos = 0;
    while ((pos = value.find(from, pos)) != std::string::npos) {
        value.replace(pos, from.size(), to);
        pos += to.size();
    }
    return value;
}

std::string pathUtf8String(const fs::path& path);

std::string urlEncode(std::string_view value)
{
    static constexpr char kHex[] = "0123456789ABCDEF";
    std::string encoded{};
    for (const unsigned char ch : value) {
        if ((ch >= 'a' && ch <= 'z')
            || (ch >= 'A' && ch <= 'Z')
            || (ch >= '0' && ch <= '9')
            || ch == '-' || ch == '_' || ch == '.' || ch == '~') {
            encoded.push_back(static_cast<char>(ch));
        } else if (ch == ' ') {
            encoded += "%20";
        } else {
            encoded.push_back('%');
            encoded.push_back(kHex[(ch >> 4) & 0x0F]);
            encoded.push_back(kHex[ch & 0x0F]);
        }
    }
    return encoded;
}

namespace {

std::string normalizeFilenameTokenText(std::string value)
{
    value = replaceAll(std::move(value), "_", " ");
    value = replaceAll(std::move(value), "-", " ");
    while (value.find("  ") != std::string::npos) {
        value = replaceAll(std::move(value), "  ", " ");
    }
    return trim(value);
}

void addUniqueFilenameGuess(std::vector<FilenameMetadataGuess>& guesses, FilenameMetadataGuess guess)
{
    guess.title = normalizeFilenameTokenText(std::move(guess.title));
    guess.artist = normalizeFilenameTokenText(std::move(guess.artist));
    if (guess.title.empty()) {
        return;
    }
    const auto duplicate = std::find_if(guesses.begin(), guesses.end(), [&](const FilenameMetadataGuess& existing) {
        return asciiLower(existing.title) == asciiLower(guess.title)
            && asciiLower(existing.artist) == asciiLower(guess.artist);
    });
    if (duplicate == guesses.end()) {
        guesses.push_back(std::move(guess));
    }
}

std::vector<std::string> splitCompactHyphenStem(std::string_view stem)
{
    std::vector<std::string> parts{};
    std::size_t start = 0;
    while (start <= stem.size()) {
        const auto end = stem.find('-', start);
        auto part = end == std::string_view::npos ? std::string(stem.substr(start)) : std::string(stem.substr(start, end - start));
        part = normalizeFilenameTokenText(std::move(part));
        if (!part.empty()) {
            parts.push_back(std::move(part));
        }
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1;
    }
    return parts;
}

std::string joinParts(const std::vector<std::string>& parts, std::size_t begin, std::size_t end)
{
    std::string joined{};
    for (std::size_t index = begin; index < end; ++index) {
        if (!joined.empty()) {
            joined += ' ';
        }
        joined += parts[index];
    }
    return trim(joined);
}

std::string stripParentheticalText(std::string value)
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

std::string normalizedMetadataMatchText(std::string value)
{
    value = stripParentheticalText(std::move(value));
    value = normalizeFilenameTokenText(std::move(value));
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

bool normalizedArtistMatches(const std::string& candidate_artist, const std::string& guess_artist)
{
    if (candidate_artist.empty() || guess_artist.empty()) {
        return false;
    }
    return candidate_artist == guess_artist
        || candidate_artist.find(guess_artist) != std::string::npos
        || guess_artist.find(candidate_artist) != std::string::npos;
}

bool decodeNextUtf8(std::string_view input, std::size_t& index, std::uint32_t& codepoint)
{
    const auto first = static_cast<unsigned char>(input[index++]);
    if (first < 0x80U) {
        codepoint = first;
        return true;
    }

    int extra = 0;
    if ((first & 0xE0U) == 0xC0U) {
        codepoint = first & 0x1FU;
        extra = 1;
    } else if ((first & 0xF0U) == 0xE0U) {
        codepoint = first & 0x0FU;
        extra = 2;
    } else if ((first & 0xF8U) == 0xF0U) {
        codepoint = first & 0x07U;
        extra = 3;
    } else {
        return false;
    }

    if (index + static_cast<std::size_t>(extra) > input.size()) {
        return false;
    }
    for (int count = 0; count < extra; ++count) {
        const auto next = static_cast<unsigned char>(input[index++]);
        if ((next & 0xC0U) != 0x80U) {
            return false;
        }
        codepoint = (codepoint << 6U) | (next & 0x3FU);
    }
    return true;
}

bool isValidUtf8(std::string_view input)
{
    std::size_t index = 0;
    std::uint32_t codepoint = 0;
    while (index < input.size()) {
        if (!decodeNextUtf8(input, index, codepoint)) {
            return false;
        }
    }
    return true;
}

bool isHexDigit(char ch)
{
    return (ch >= '0' && ch <= '9')
        || (ch >= 'a' && ch <= 'f')
        || (ch >= 'A' && ch <= 'F');
}

std::optional<std::uint32_t> parseHexCodepoint(std::string_view value)
{
    if (value.size() != 4 || !std::all_of(value.begin(), value.end(), isHexDigit)) {
        return std::nullopt;
    }
    std::uint32_t codepoint = 0;
    for (const char ch : value) {
        codepoint <<= 4U;
        if (ch >= '0' && ch <= '9') {
            codepoint += static_cast<std::uint32_t>(ch - '0');
        } else if (ch >= 'a' && ch <= 'f') {
            codepoint += static_cast<std::uint32_t>(ch - 'a' + 10);
        } else {
            codepoint += static_cast<std::uint32_t>(ch - 'A' + 10);
        }
    }
    return codepoint;
}

void appendUtf8(std::string& output, std::uint32_t codepoint)
{
    if (codepoint <= 0x7FU) {
        output.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FFU) {
        output.push_back(static_cast<char>(0xC0U | ((codepoint >> 6U) & 0x1FU)));
        output.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
    } else if (codepoint <= 0xFFFFU) {
        output.push_back(static_cast<char>(0xE0U | ((codepoint >> 12U) & 0x0FU)));
        output.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU)));
        output.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
    } else {
        output.push_back(static_cast<char>(0xF0U | ((codepoint >> 18U) & 0x07U)));
        output.push_back(static_cast<char>(0x80U | ((codepoint >> 12U) & 0x3FU)));
        output.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU)));
        output.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
    }
}

std::optional<unsigned char> windows1252ByteForCodepoint(std::uint32_t codepoint)
{
    if (codepoint <= 0xFFU) {
        return static_cast<unsigned char>(codepoint);
    }

    switch (codepoint) {
    case 0x20ACU: return static_cast<unsigned char>(0x80U);
    case 0x201AU: return static_cast<unsigned char>(0x82U);
    case 0x0192U: return static_cast<unsigned char>(0x83U);
    case 0x201EU: return static_cast<unsigned char>(0x84U);
    case 0x2026U: return static_cast<unsigned char>(0x85U);
    case 0x2020U: return static_cast<unsigned char>(0x86U);
    case 0x2021U: return static_cast<unsigned char>(0x87U);
    case 0x02C6U: return static_cast<unsigned char>(0x88U);
    case 0x2030U: return static_cast<unsigned char>(0x89U);
    case 0x0160U: return static_cast<unsigned char>(0x8AU);
    case 0x2039U: return static_cast<unsigned char>(0x8BU);
    case 0x0152U: return static_cast<unsigned char>(0x8CU);
    case 0x017DU: return static_cast<unsigned char>(0x8EU);
    case 0x2018U: return static_cast<unsigned char>(0x91U);
    case 0x2019U: return static_cast<unsigned char>(0x92U);
    case 0x201CU: return static_cast<unsigned char>(0x93U);
    case 0x201DU: return static_cast<unsigned char>(0x94U);
    case 0x2022U: return static_cast<unsigned char>(0x95U);
    case 0x2013U: return static_cast<unsigned char>(0x96U);
    case 0x2014U: return static_cast<unsigned char>(0x97U);
    case 0x02DCU: return static_cast<unsigned char>(0x98U);
    case 0x2122U: return static_cast<unsigned char>(0x99U);
    case 0x0161U: return static_cast<unsigned char>(0x9AU);
    case 0x203AU: return static_cast<unsigned char>(0x9BU);
    case 0x0153U: return static_cast<unsigned char>(0x9CU);
    case 0x017EU: return static_cast<unsigned char>(0x9EU);
    case 0x0178U: return static_cast<unsigned char>(0x9FU);
    default: return std::nullopt;
    }
}

std::string repairLatin1MojibakeUtf8(std::string value)
{
    std::string bytes{};
    bytes.reserve(value.size());
    bool saw_extended_byte = false;
    std::size_t index = 0;
    while (index < value.size()) {
        std::uint32_t codepoint = 0;
        if (!decodeNextUtf8(value, index, codepoint)) {
            return value;
        }
        const auto byte = windows1252ByteForCodepoint(codepoint);
        if (!byte) {
            return value;
        }
        if (*byte >= 0x80U) {
            saw_extended_byte = true;
        }
        bytes.push_back(static_cast<char>(*byte));
    }

    if (!saw_extended_byte || !isValidUtf8(bytes)) {
        return value;
    }
    return bytes;
}

std::string repairBareUnicodeEscapes(std::string value)
{
    int escape_count = 0;
    std::string decoded{};
    decoded.reserve(value.size());
    for (std::size_t index = 0; index < value.size();) {
        if (value[index] == 'u' && index + 5 <= value.size()) {
            if (const auto codepoint = parseHexCodepoint(std::string_view(value).substr(index + 1, 4))) {
                appendUtf8(decoded, *codepoint);
                index += 5;
                ++escape_count;
                continue;
            }
        }
        decoded.push_back(value[index]);
        ++index;
    }
    return escape_count >= 2 ? decoded : value;
}

} // namespace

std::optional<std::string> parseJsonStringAt(std::string_view json, std::size_t quote_pos)
{
    if (quote_pos >= json.size() || json[quote_pos] != '"') {
        return std::nullopt;
    }

    std::size_t cursor = quote_pos + 1;
    std::string result{};
    while (cursor < json.size()) {
        const char ch = json[cursor++];
        if (ch == '"') {
            return result;
        }
        if (ch != '\\') {
            result.push_back(ch);
            continue;
        }
        if (cursor >= json.size()) {
            return std::nullopt;
        }
        const char escaped = json[cursor++];
        switch (escaped) {
        case 'n':
            result.push_back('\n');
            break;
        case 'r':
            result.push_back('\r');
            break;
        case 't':
            result.push_back('\t');
            break;
        case '"':
            result.push_back('"');
            break;
        case '\\':
            result.push_back('\\');
            break;
        case '/':
            result.push_back('/');
            break;
        case 'u':
            if (cursor + 4 > json.size()) {
                return std::nullopt;
            }
            if (const auto codepoint = parseHexCodepoint(json.substr(cursor, 4))) {
                appendUtf8(result, *codepoint);
                cursor += 4;
            } else {
                result.push_back('u');
            }
            break;
        default:
            result.push_back(escaped);
            break;
        }
    }
    return std::nullopt;
}

std::string repairMetadataText(std::string value)
{
    value = repairBareUnicodeEscapes(std::move(value));
    value = repairLatin1MojibakeUtf8(std::move(value));
    return value;
}

std::vector<FilenameMetadataGuess> fallbackMetadataGuessesFromPath(const fs::path& path)
{
    std::vector<FilenameMetadataGuess> guesses{};
    const std::string stem = repairMetadataText(pathUtf8String(path.stem()));
    const std::string parent_directory = asciiLower(pathUtf8String(path.parent_path().filename()));
    const bool flat_music_root = parent_directory == "music";
    const auto spaced_separator = stem.find(" - ");
    if (spaced_separator != std::string::npos) {
        addUniqueFilenameGuess(guesses, FilenameMetadataGuess{
            stem.substr(spaced_separator + 3),
            stem.substr(0, spaced_separator)});
    } else if (stem.find('-') != std::string::npos) {
        const auto parts = splitCompactHyphenStem(stem);
        if (parts.size() >= 2) {
            const std::size_t max_artist_parts = std::min<std::size_t>(2, parts.size() - 1);
            for (std::size_t split = 1; split <= max_artist_parts; ++split) {
                addUniqueFilenameGuess(guesses, FilenameMetadataGuess{
                    joinParts(parts, split, parts.size()),
                    joinParts(parts, 0, split)});
            }
        }
    }

    addUniqueFilenameGuess(guesses, FilenameMetadataGuess{
        stem,
        flat_music_root ? std::string{} : repairMetadataText(pathUtf8String(path.parent_path().parent_path().filename()))});
    return guesses;
}

bool metadataCompatibleWithFilename(const fs::path& path, const app::TrackMetadata& metadata)
{
    if (!metadata.title || metadata.title->empty()) {
        return true;
    }
    if (pathUtf8String(path.stem()).find('-') == std::string::npos) {
        return true;
    }

    const auto guesses = fallbackMetadataGuessesFromPath(path);
    if (guesses.empty()) {
        return true;
    }

    const std::string metadata_title = normalizedMetadataMatchText(*metadata.title);
    const std::string metadata_artist = normalizedMetadataMatchText(metadata.artist.value_or(""));
    if (metadata_title.empty()) {
        return true;
    }

    for (const auto& guess : guesses) {
        const std::string guess_title = normalizedMetadataMatchText(guess.title);
        const std::string guess_artist = normalizedMetadataMatchText(guess.artist);
        if (guess_title.empty()) {
            continue;
        }
        if (metadata_title == guess_title) {
            return true;
        }
        const bool fuzzy_title_match =
            metadata_title.find(guess_title) != std::string::npos
            || guess_title.find(metadata_title) != std::string::npos;
        if (fuzzy_title_match && normalizedArtistMatches(metadata_artist, guess_artist)) {
            return true;
        }
    }

    return false;
}

std::string fallbackTitleFromPath(const fs::path& path)
{
    const auto guesses = fallbackMetadataGuessesFromPath(path);
    return guesses.empty() ? normalizeFilenameTokenText(pathUtf8String(path.stem())) : guesses.front().title;
}

std::string fallbackAlbumFromPath(const fs::path& path)
{
    return trim(pathUtf8String(path.parent_path().filename()));
}

std::string fallbackArtistFromPath(const fs::path& path)
{
    const auto guesses = fallbackMetadataGuessesFromPath(path);
    return guesses.empty() ? trim(pathUtf8String(path.parent_path().parent_path().filename())) : guesses.front().artist;
}

std::string pathUtf8String(const fs::path& path)
{
#if defined(_WIN32)
    const auto u8 = path.u8string();
    return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
#else
    return path.string();
#endif
}

fs::path projectRoot()
{
    return fs::path(LOFIBOX_PROJECT_SOURCE_DIR);
}

fs::path helperScriptPath(std::string_view script_name)
{
    std::vector<fs::path> candidates{};
    if (const char* helper_dir = std::getenv("LOFIBOX_HELPER_DIR"); helper_dir != nullptr && *helper_dir != '\0') {
        candidates.emplace_back(fs::path(helper_dir) / script_name);
    }
    candidates.emplace_back(fs::path(LOFIBOX_INSTALLED_HELPER_DIR) / script_name);
    candidates.emplace_back(projectRoot() / "scripts" / script_name);

    for (const auto& candidate : candidates) {
        std::error_code ec{};
        if (fs::exists(candidate, ec) && !ec) {
            return candidate;
        }
    }
    return candidates.front();
}

fs::path tagWriterScriptPath()
{
    return helperScriptPath("tag_writer.py");
}

fs::path remoteMediaToolScriptPath()
{
    return helperScriptPath("remote_media_tool.py");
}

std::string jsonEscape(std::string_view value)
{
    std::string escaped{};
    for (const char ch : value) {
        switch (ch) {
        case '\\': escaped += "\\\\"; break;
        case '"': escaped += "\\\""; break;
        case '\n': escaped += "\\n"; break;
        case '\r': escaped += "\\r"; break;
        case '\t': escaped += "\\t"; break;
        default: escaped.push_back(ch); break;
        }
    }
    return escaped;
}

std::vector<fs::path> configuredMediaRoots()
{
    std::vector<fs::path> roots{};
#if defined(_WIN32)
    wchar_t* raw_env_value = nullptr;
    std::size_t env_length = 0;
    if (_wdupenv_s(&raw_env_value, &env_length, L"LOFIBOX_MEDIA_ROOT") != 0 || raw_env_value == nullptr || *raw_env_value == L'\0') {
        std::free(raw_env_value);
        return roots;
    }
    std::wstring text(raw_env_value);
    std::free(raw_env_value);
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t end = text.find(L';', start);
        std::wstring segment = end == std::wstring::npos ? text.substr(start) : text.substr(start, end - start);
        while (!segment.empty() && std::iswspace(segment.front()) != 0) {
            segment.erase(segment.begin());
        }
        while (!segment.empty() && std::iswspace(segment.back()) != 0) {
            segment.pop_back();
        }
        if (!segment.empty()) {
            std::error_code ec{};
            fs::path path = fs::path(segment);
            const auto normalized = fs::weakly_canonical(path, ec);
            roots.push_back(ec ? path.lexically_normal() : normalized);
        }
        if (end == std::wstring::npos) {
            break;
        }
        start = end + 1;
    }
#else
    const char* env_value = std::getenv("LOFIBOX_MEDIA_ROOT");
    if (env_value == nullptr || *env_value == '\0') {
        return roots;
    }
    std::string text(env_value);
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t end = text.find(';', start);
        std::string segment = end == std::string::npos ? text.substr(start) : text.substr(start, end - start);
        segment = trim(std::move(segment));
        if (!segment.empty()) {
            std::error_code ec{};
            fs::path path = fs::path(segment);
            const auto normalized = fs::weakly_canonical(path, ec);
            roots.push_back(ec ? path.lexically_normal() : normalized);
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
#endif
    return roots;
}

bool pathEquivalentToAnyRoot(const fs::path& path, const std::vector<fs::path>& roots)
{
    std::error_code ec{};
    const auto normalized = fs::weakly_canonical(path, ec);
    const auto candidate = ec ? path.lexically_normal() : normalized;
    return std::any_of(roots.begin(), roots.end(), [&](const fs::path& root) {
        return candidate == root;
    });
}

std::optional<bool> parseJsonBool(std::string_view json, std::string_view marker)
{
    const auto pos = json.find(marker);
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }

    std::size_t cursor = pos + marker.size();
    while (cursor < json.size() && std::isspace(static_cast<unsigned char>(json[cursor])) != 0) {
        ++cursor;
    }
    if (json.substr(cursor, 4) == "true") {
        return true;
    }
    if (json.substr(cursor, 5) == "false") {
        return false;
    }
    return std::nullopt;
}

std::optional<int> parseJsonInt(std::string_view json, std::string_view marker)
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
    while (end < json.size() && (std::isdigit(static_cast<unsigned char>(json[end])) != 0 || json[end] == '-')) {
        ++end;
    }
    if (end == cursor) {
        return std::nullopt;
    }
    try {
        return std::stoi(std::string(json.substr(cursor, end - cursor)));
    } catch (...) {
        return std::nullopt;
    }
}

bool hasCoreMetadata(const app::TrackMetadata& metadata)
{
    return metadata.title.has_value() && metadata.artist.has_value() && metadata.album.has_value();
}


std::optional<std::string> extractJsonString(std::string_view json, std::string_view marker, std::size_t start)
{
    const auto pos = json.find(marker, start);
    if (pos == std::string_view::npos) {
        return std::nullopt;
    }
    return parseJsonStringAt(json, pos + marker.size() - 1);
}

std::size_t findJsonBlock(std::string_view json, std::string_view marker)
{
    return json.find(marker);
}

void respectMusicBrainzRateLimit(SharedRuntimeCache& cache)
{
    const auto now = std::chrono::steady_clock::now();
    if (cache.last_musicbrainz_request != std::chrono::steady_clock::time_point{}) {
        const auto elapsed = now - cache.last_musicbrainz_request;
        const auto min_gap = std::chrono::seconds(1);
        if (elapsed < min_gap) {
            std::this_thread::sleep_for(min_gap - elapsed);
        }
    }
    cache.last_musicbrainz_request = std::chrono::steady_clock::now();
}

app::TrackMetadata parseMetadataOutput(const std::string& output)
{
    app::TrackMetadata metadata{};
    auto setIfEmpty = [](std::optional<std::string>& field, std::optional<std::string> value) {
        if (!field && value) {
            field = std::move(value);
        }
    };
    std::size_t start = 0;
    while (start < output.size()) {
        const std::size_t end = output.find('\n', start);
        const std::string line = trim(output.substr(start, end == std::string::npos ? std::string::npos : end - start));
        const std::string lower_line = asciiLower(line);

        if (lower_line.rfind("duration=", 0) == 0) {
            try {
                const double seconds = std::stod(line.substr(9));
                metadata.duration_seconds = std::max(0, static_cast<int>(std::lround(seconds)));
            } catch (...) {
            }
        } else if (lower_line.rfind("tag:title=", 0) == 0) {
            setIfEmpty(metadata.title, nonEmptyValue(repairMetadataText(line.substr(10))));
        } else if (lower_line.rfind("tag:artist=", 0) == 0) {
            setIfEmpty(metadata.artist, nonEmptyValue(repairMetadataText(line.substr(11))));
        } else if (lower_line.rfind("tag:album=", 0) == 0) {
            setIfEmpty(metadata.album, nonEmptyValue(repairMetadataText(line.substr(10))));
        } else if (lower_line.rfind("tag:genre=", 0) == 0) {
            setIfEmpty(metadata.genre, nonEmptyValue(repairMetadataText(line.substr(10))));
        } else if (lower_line.rfind("tag:composer=", 0) == 0) {
            setIfEmpty(metadata.composer, nonEmptyValue(repairMetadataText(line.substr(13))));
        }

        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    return metadata;
}

std::optional<std::string> extractLyricsText(std::string_view json, std::string_view field)
{
    return extractJsonString(json, field);
}


} // namespace lofibox::platform::host::runtime_detail
