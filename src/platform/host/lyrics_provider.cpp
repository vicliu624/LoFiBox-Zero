// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/runtime_provider_factories.h"

#include <cctype>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "platform/host/runtime_enrichment_clients.h"
#include "platform/host/runtime_logger.h"

namespace fs = std::filesystem;

namespace lofibox::platform::host {
namespace {
using namespace runtime_detail;

constexpr int kLyricsLookupVersion = 5;

[[nodiscard]] bool lrcTimestampStartsAt(std::string_view value, std::size_t index)
{
    if (index + 6 >= value.size() || value[index] != '[') {
        return false;
    }
    std::size_t cursor = index + 1;
    bool has_minutes = false;
    while (cursor < value.size() && std::isdigit(static_cast<unsigned char>(value[cursor])) != 0) {
        has_minutes = true;
        ++cursor;
    }
    if (!has_minutes || cursor >= value.size() || value[cursor] != ':') {
        return false;
    }
    ++cursor;
    int second_digits = 0;
    while (cursor < value.size() && std::isdigit(static_cast<unsigned char>(value[cursor])) != 0 && second_digits < 2) {
        ++second_digits;
        ++cursor;
    }
    return second_digits == 2;
}

[[nodiscard]] std::string normalizeLyricLineBreaks(std::string value)
{
    int timestamp_count = 0;
    for (std::size_t index = 0; index < value.size(); ++index) {
        if (lrcTimestampStartsAt(value, index)) {
            ++timestamp_count;
        }
    }
    if (timestamp_count < 2 || value.find('\n') != std::string::npos) {
        return value;
    }

    std::string normalized{};
    normalized.reserve(value.size() + static_cast<std::size_t>(timestamp_count));
    for (std::size_t index = 0; index < value.size(); ++index) {
        if (index > 0 && lrcTimestampStartsAt(value, index)) {
            if (!normalized.empty() && normalized.back() == 'n') {
                normalized.pop_back();
            }
            normalized.push_back('\n');
        }
        normalized.push_back(value[index]);
    }
    return normalized;
}

[[nodiscard]] std::string normalizeLyricsMatchText(std::string value)
{
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

[[nodiscard]] std::string stripLrcDecorations(std::string line)
{
    line = trim(std::move(line));
    while (line.size() >= 3 && line.front() == '[') {
        const auto close = line.find(']');
        if (close == std::string::npos) {
            break;
        }
        line = trim(line.substr(close + 1));
    }
    return line;
}

[[nodiscard]] bool isLrcMetadataLine(std::string_view line)
{
    return line.size() >= 5
        && line.front() == '['
        && line.find(':') != std::string_view::npos
        && line.find(']') != std::string_view::npos
        && !lrcTimestampStartsAt(line, 0);
}

[[nodiscard]] std::optional<std::string> firstContentLyricLine(const app::TrackLyrics& lyrics)
{
    const std::string source = lyrics.synced.value_or(lyrics.plain.value_or(""));
    std::size_t start = 0;
    while (start <= source.size()) {
        const auto end = source.find('\n', start);
        auto line = source.substr(start, end == std::string::npos ? std::string::npos : end - start);
        line = trim(std::move(line));
        if (!line.empty() && !isLrcMetadataLine(line)) {
            line = stripLrcDecorations(std::move(line));
            if (!line.empty()) {
                return line;
            }
        }
        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }
    return std::nullopt;
}

[[nodiscard]] bool fuzzyMetadataTextMatches(const std::string& candidate, const std::string& expected)
{
    const auto normalized_candidate = normalizeLyricsMatchText(candidate);
    const auto normalized_expected = normalizeLyricsMatchText(expected);
    if (normalized_candidate.empty() || normalized_expected.empty()) {
        return false;
    }
    return normalized_candidate == normalized_expected
        || normalized_candidate.find(normalized_expected) != std::string::npos
        || normalized_expected.find(normalized_candidate) != std::string::npos;
}

[[nodiscard]] bool titleArtistPairMatchesTrack(
    const std::string& left,
    const std::string& right,
    const fs::path& path,
    const app::TrackMetadata& metadata)
{
    std::vector<FilenameMetadataGuess> guesses = fallbackMetadataGuessesFromPath(path);
    if (metadata.title && !metadata.title->empty()) {
        guesses.push_back(FilenameMetadataGuess{*metadata.title, metadata.artist.value_or("")});
    }

    for (const auto& guess : guesses) {
        const bool left_artist_right_title = fuzzyMetadataTextMatches(left, guess.artist)
            && fuzzyMetadataTextMatches(right, guess.title);
        const bool left_title_right_artist = fuzzyMetadataTextMatches(left, guess.title)
            && fuzzyMetadataTextMatches(right, guess.artist);
        if (left_artist_right_title || left_title_right_artist) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool embeddedLyricsLookMismatched(
    const app::TrackLyrics& lyrics,
    const fs::path& path,
    const app::TrackMetadata& metadata)
{
    const auto first_line = firstContentLyricLine(lyrics);
    if (!first_line) {
        return false;
    }

    const std::vector<std::string_view> separators{" - ", " – ", " — ", " / "};
    for (const auto separator : separators) {
        const auto separator_pos = first_line->find(separator);
        if (separator_pos == std::string::npos) {
            continue;
        }
        const auto left = trim(first_line->substr(0, separator_pos));
        const auto right = trim(first_line->substr(separator_pos + separator.size()));
        if (left.empty() || right.empty()) {
            continue;
        }
        return !titleArtistPairMatchesTrack(left, right, path, metadata);
    }
    return false;
}

class EmbeddedLyricsReader final {
public:
    EmbeddedLyricsReader()
    {
#if defined(_WIN32)
        executable_path_ = resolveExecutablePath(L"FFPROBE_PATH", L"ffprobe.exe");
        python_path_ = resolveExecutablePath(L"PYTHON_EXECUTABLE", L"python.exe");
#elif defined(__linux__)
        executable_path_ = resolveExecutablePath("FFPROBE_PATH", "ffprobe");
        python_path_ = resolveExecutablePath("PYTHON_EXECUTABLE", "python3");
#endif
    }

    [[nodiscard]] app::TrackLyrics read(const fs::path& path) const
    {
        if (auto lyrics = readWithMutagen(path); lyrics.plain || lyrics.synced) {
            return lyrics;
        }
        return readWithFfprobe(path);
    }

private:
    [[nodiscard]] app::TrackLyrics readWithFfprobe(const fs::path& path) const
    {
        app::TrackLyrics lyrics{};
        if (!executable_path_ || !fs::exists(path)) {
            return lyrics;
        }

        const std::vector<std::string> args{
            "-v",
            "error",
            "-select_streams",
            "a:0",
            "-show_entries",
            "stream_tags=lyrics,LYRICS,syncedlyrics,SYNCEDLYRICS,unsyncedlyrics,UNSYNCEDLYRICS",
            "-of",
            "json=compact=1",
            pathUtf8String(path)};

        const auto output = captureProcessOutput(*executable_path_, args);
        if (!output) {
            return lyrics;
        }

        const auto extract_tag = [&](std::string_view name) -> std::optional<std::string> {
            if (auto value = extractJsonString(*output, "\"" + std::string(name) + "\": \"")) {
                return nonEmptyValue(*value);
            }
            if (auto value = extractJsonString(*output, "\"" + std::string(name) + "\":\"")) {
                return nonEmptyValue(*value);
            }
            return std::nullopt;
        };

        lyrics.synced = extract_tag("syncedlyrics");
        if (!lyrics.synced) {
            lyrics.synced = extract_tag("SYNCEDLYRICS");
        }
        if (lyrics.synced) {
            lyrics.synced = normalizeLyricLineBreaks(std::move(*lyrics.synced));
        }

        lyrics.plain = extract_tag("lyrics");
        if (!lyrics.plain) {
            lyrics.plain = extract_tag("LYRICS");
        }
        if (!lyrics.plain) {
            lyrics.plain = extract_tag("unsyncedlyrics");
        }
        if (!lyrics.plain) {
            lyrics.plain = extract_tag("UNSYNCEDLYRICS");
        }
        if (lyrics.plain) {
            lyrics.plain = normalizeLyricLineBreaks(std::move(*lyrics.plain));
        }

        if (lyrics.plain || lyrics.synced) {
            lyrics.source = "EMBEDDED";
        }
        return lyrics;
    }

    [[nodiscard]] app::TrackLyrics readWithMutagen(const fs::path& path) const
    {
        app::TrackLyrics lyrics{};
        if (!python_path_ || !fs::exists(path)) {
            return lyrics;
        }

        const std::string script =
            "import sys, mutagen\n"
            "audio = mutagen.File(sys.argv[1], easy=False)\n"
            "if audio is None:\n"
            "    raise SystemExit(0)\n"
            "plain = ''\n"
            "synced = ''\n"
            "tags = getattr(audio, 'tags', None)\n"
            "if tags:\n"
            "    for key, value in tags.items():\n"
            "        lk = str(key).lower()\n"
            "        if lk.startswith('uslt') and not plain:\n"
            "            plain = str(getattr(value, 'text', '') or '')\n"
            "        elif lk == 'lyrics' and not plain:\n"
            "            if isinstance(value, list): plain = str(value[0]) if value else ''\n"
            "            else: plain = str(value)\n"
            "        elif 'syncedlyrics' in lk and not synced:\n"
            "            text = getattr(value, 'text', value)\n"
            "            if isinstance(text, list): synced = str(text[0]) if text else ''\n"
            "            else: synced = str(text)\n"
            "for name in ('lyrics', 'LYRICS'):\n"
            "    if not plain and hasattr(audio, 'get'):\n"
            "        value = audio.get(name)\n"
            "        if value:\n"
            "            plain = str(value[0]) if isinstance(value, list) else str(value)\n"
            "for name in ('syncedlyrics', 'SYNCEDLYRICS'):\n"
            "    if not synced and hasattr(audio, 'get'):\n"
            "        value = audio.get(name)\n"
            "        if value:\n"
            "            synced = str(value[0]) if isinstance(value, list) else str(value)\n"
            "if plain:\n"
            "    print('plain=' + plain.replace('\\\\', '\\\\\\\\').replace('\\r', '\\\\r').replace('\\n', '\\\\n'))\n"
            "if synced:\n"
            "    print('synced=' + synced.replace('\\\\', '\\\\\\\\').replace('\\r', '\\\\r').replace('\\n', '\\\\n'))\n";

        const std::vector<std::string> args{"-c", script, pathUtf8String(path)};
        const auto output = captureProcessOutput(*python_path_, args);
        if (!output) {
            return lyrics;
        }

        const auto unescape = [](std::string value) {
            std::string result{};
            bool escape = false;
            for (const char ch : value) {
                if (escape) {
                    switch (ch) {
                    case 'n': result.push_back('\n'); break;
                    case 'r': result.push_back('\r'); break;
                    case 't': result.push_back('\t'); break;
                    case '\\': result.push_back('\\'); break;
                    default: result.push_back(ch); break;
                    }
                    escape = false;
                } else if (ch == '\\') {
                    escape = true;
                } else {
                    result.push_back(ch);
                }
            }
            return result;
        };

        std::size_t start = 0;
        while (start <= output->size()) {
            const auto end = output->find('\n', start);
            const auto line = output->substr(start, end == std::string::npos ? std::string::npos : end - start);
            if (line.rfind("plain=", 0) == 0) {
                if (auto value = nonEmptyValue(unescape(line.substr(6)))) {
                    lyrics.plain = normalizeLyricLineBreaks(std::move(*value));
                }
            } else if (line.rfind("synced=", 0) == 0) {
                if (auto value = nonEmptyValue(unescape(line.substr(7)))) {
                    lyrics.synced = normalizeLyricLineBreaks(std::move(*value));
                }
            }
            if (end == std::string::npos) {
                break;
            }
            start = end + 1;
        }

        if (lyrics.plain || lyrics.synced) {
            lyrics.source = "EMBEDDED";
        }
        return lyrics;
    }

    std::optional<fs::path> executable_path_{};
    std::optional<fs::path> python_path_{};
};

class OnlineLyricsProviderChain final {
public:
    [[nodiscard]] bool available() const
    {
        return lrclib_.available() || lyrics_ovh_.available();
    }

    [[nodiscard]] std::string displayName() const
    {
        return "LRCLIB -> lyrics.ovh";
    }

    [[nodiscard]] app::TrackLyrics fetch(const fs::path& path, const app::TrackMetadata& metadata) const
    {
        if (lrclib_.available()) {
            if (auto lyrics = lrclib_.fetch(path, metadata); lyrics.plain || lyrics.synced) {
                return lyrics;
            }
        }

        if (lyrics_ovh_.available()) {
            if (auto lyrics = lyrics_ovh_.fetch(path, metadata); lyrics.plain || lyrics.synced) {
                return lyrics;
            }
        }

        return {};
    }

private:
    LrclibClient lrclib_{};
    LyricsOvhClient lyrics_ovh_{};
};

class HostLyricsProvider final : public app::LyricsProvider {
public:
    explicit HostLyricsProvider(
        std::shared_ptr<SharedRuntimeCache> cache,
        std::shared_ptr<app::ConnectivityProvider> connectivity,
        std::shared_ptr<app::TagWriter> tag_writer,
        std::shared_ptr<app::TrackIdentityProvider> track_identity_provider)
        : cache_(std::move(cache))
        , connectivity_(std::move(connectivity))
        , tag_writer_(std::move(tag_writer))
        , track_identity_provider_(std::move(track_identity_provider))
    {
    }

    [[nodiscard]] bool available() const override
    {
        return online_lyrics_chain_.available();
    }

    [[nodiscard]] std::string displayName() const override
    {
        return "Embedded + " + online_lyrics_chain_.displayName();
    }

    [[nodiscard]] app::TrackLyrics fetch(const fs::path& path, const app::TrackMetadata& metadata) const override
    {
        app::TrackLyrics lyrics{};
        if (!cache_) {
            return lyrics;
        }

        const auto key = cacheKeyForPath(path);
        auto& entry = cache_->entries[key];
        bool embedded_lyrics_rejected = false;
        const auto embedded_lyrics = embedded_reader_.read(path);
        if (embedded_lyrics.plain || embedded_lyrics.synced) {
            if (embeddedLyricsLookMismatched(embedded_lyrics, path, metadata)) {
                embedded_lyrics_rejected = true;
                entry.lyrics = {};
                logRuntime(RuntimeLogLevel::Warn, "lyrics", "Embedded lyrics rejected as mismatched for " + pathUtf8String(path));
            } else {
                entry.lyrics = embedded_lyrics;
                storeLyricsCache(*cache_, key, entry.lyrics);
                logRuntime(RuntimeLogLevel::Debug, "lyrics", "Embedded lyrics hit for " + pathUtf8String(path));
                return entry.lyrics;
            }
        }

        if (!entry.online_lyrics_attempted && entry.lyrics_lookup_version >= kLyricsLookupVersion && fs::exists(lyricsCachePath(*cache_, key))) {
            entry.lyrics = loadLyricsCache(*cache_, key);
        }
        if (entry.lyrics.plain || entry.lyrics.synced) {
            return entry.lyrics;
        }

        const bool lyrics_lookup_current = entry.online_lyrics_attempted && entry.lyrics_lookup_version >= kLyricsLookupVersion;
        if (!online_lyrics_chain_.available() || !connectivity_ || !connectivity_->connected() || lyrics_lookup_current) {
            if (lyrics_lookup_current) {
                logRuntime(RuntimeLogLevel::Debug, "lyrics", "Online lyrics skipped; current miss cache for " + pathUtf8String(path));
            }
            return entry.lyrics;
        }

        app::TrackMetadata lyrics_metadata = metadata;
        app::TrackIdentity identity{};
        if (track_identity_provider_ && track_identity_provider_->available()) {
            identity = track_identity_provider_->resolve(path, metadata);
            if (identity.found) {
                if (identity.metadata.title) lyrics_metadata.title = identity.metadata.title;
                if (identity.metadata.artist) lyrics_metadata.artist = identity.metadata.artist;
                if (identity.metadata.album) lyrics_metadata.album = identity.metadata.album;
                if (identity.metadata.duration_seconds) lyrics_metadata.duration_seconds = identity.metadata.duration_seconds;
                entry.identity = identity;
                logRuntime(RuntimeLogLevel::Debug, "lyrics", "Lyrics lookup using track identity from " + identity.source + " for " + pathUtf8String(path));
            }
        }

        entry.lyrics = online_lyrics_chain_.fetch(path, lyrics_metadata);

        entry.online_lyrics_attempted = true;
        entry.lyrics_lookup_version = kLyricsLookupVersion;
        storeMetadataCache(*cache_, key, entry);
        if (entry.lyrics.plain || entry.lyrics.synced) {
            storeLyricsCache(*cache_, key, entry.lyrics);
            logRuntime(RuntimeLogLevel::Info, "lyrics", "Online lyrics hit for " + pathUtf8String(path));
            if (tag_writer_ && tag_writer_->available()) {
                app::TagWriteRequest request{};
                request.lyrics = entry.lyrics;
                if (identity.found) {
                    request.identity = identity;
                }
                request.only_fill_missing = !embedded_lyrics_rejected;
                if (entry.lyrics.source == "LRCLIB") {
                    if (tag_writer_->write(path, request)) {
                        logRuntime(RuntimeLogLevel::Info, "lyrics", "Wrote lyrics back to file: " + pathUtf8String(path));
                    } else {
                        logRuntime(RuntimeLogLevel::Warn, "lyrics", "Failed to write lyrics back to file: " + pathUtf8String(path));
                    }
                } else {
                    logRuntime(RuntimeLogLevel::Info, "lyrics", "Skipped file lyrics writeback for non-authoritative source: " + entry.lyrics.source);
                }
            }
        } else {
            logRuntime(RuntimeLogLevel::Info, "lyrics", "No online lyrics found for: " + pathUtf8String(path));
        }

        return entry.lyrics;
    }

private:
    std::shared_ptr<SharedRuntimeCache> cache_{};
    std::shared_ptr<app::ConnectivityProvider> connectivity_{};
    std::shared_ptr<app::TagWriter> tag_writer_{};
    std::shared_ptr<app::TrackIdentityProvider> track_identity_provider_{};
    EmbeddedLyricsReader embedded_reader_{};
    OnlineLyricsProviderChain online_lyrics_chain_{};
};

} // namespace

std::shared_ptr<app::LyricsProvider> createHostLyricsProvider(
    std::shared_ptr<runtime_detail::SharedRuntimeCache> cache,
    std::shared_ptr<app::ConnectivityProvider> connectivity,
    std::shared_ptr<app::TagWriter> tag_writer,
    std::shared_ptr<app::TrackIdentityProvider> track_identity_provider)
{
    return std::make_shared<HostLyricsProvider>(
        std::move(cache),
        std::move(connectivity),
        std::move(tag_writer),
        std::move(track_identity_provider));
}

} // namespace lofibox::platform::host
