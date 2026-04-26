// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/widgets/lyrics_layout.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <optional>
#include <sstream>

namespace lofibox::ui::widgets {
namespace {

std::string trimText(std::string value)
{
    const auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

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

std::string normalizeLyricSourceForDisplay(std::string value)
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

std::optional<double> parseLyricTimestamp(std::string_view line, std::size_t& tag_end)
{
    tag_end = 0;
    if (line.size() < 7 || line.front() != '[') {
        return std::nullopt;
    }
    const auto close = line.find(']');
    const auto colon = line.find(':');
    if (close == std::string_view::npos || colon == std::string_view::npos || colon > close) {
        return std::nullopt;
    }
    const std::string minutes_string{line.substr(1, colon - 1)};
    const std::string seconds_string{line.substr(colon + 1, close - colon - 1)};
    char* minute_end = nullptr;
    char* second_end = nullptr;
    const long minutes = std::strtol(minutes_string.c_str(), &minute_end, 10);
    const double seconds = std::strtod(seconds_string.c_str(), &second_end);
    if (minutes_string.empty() || seconds_string.empty() || minute_end == minutes_string.c_str() || *minute_end != '\0' || second_end == seconds_string.c_str() || *second_end != '\0') {
        return std::nullopt;
    }
    tag_end = close + 1;
    return static_cast<double>(minutes * 60) + seconds;
}

[[nodiscard]] bool isLrcMetadataLine(std::string_view line) noexcept
{
    if (line.size() < 4 || line.front() != '[') {
        return false;
    }
    const auto close = line.find(']');
    const auto colon = line.find(':');
    return close != std::string_view::npos && colon != std::string_view::npos && colon < close;
}

} // namespace

std::vector<LyricDisplayLine> lyricDisplayLines(const LyricsContent& lyrics)
{
    std::vector<LyricDisplayLine> lines{};
    const auto& source = lyrics.synced && !lyrics.synced->empty() ? lyrics.synced : lyrics.plain;
    if (!source || source->empty()) {
        return lines;
    }
    std::istringstream input{normalizeLyricSourceForDisplay(*source)};
    std::string line{};
    while (std::getline(input, line)) {
        line = trimText(std::move(line));
        if (line.empty()) {
            continue;
        }
        LyricDisplayLine display_line{};
        std::size_t tag_end = 0;
        if (const auto timestamp = parseLyricTimestamp(line, tag_end)) {
            display_line.timestamp_seconds = *timestamp;
            display_line.text = trimText(line.substr(tag_end));
        } else if (isLrcMetadataLine(line)) {
            continue;
        } else {
            display_line.text = std::move(line);
        }
        if (!display_line.text.empty()) {
            lines.push_back(std::move(display_line));
        }
    }
    return lines;
}

int activeLyricIndex(const std::vector<LyricDisplayLine>& lines, double elapsed_seconds, int duration_seconds)
{
    if (lines.empty()) {
        return 0;
    }
    const bool has_timestamps = std::any_of(lines.begin(), lines.end(), [](const LyricDisplayLine& line) {
        return line.timestamp_seconds >= 0.0;
    });
    if (has_timestamps) {
        int active = 0;
        for (int index = 0; index < static_cast<int>(lines.size()); ++index) {
            if (lines[static_cast<std::size_t>(index)].timestamp_seconds >= 0.0) {
                active = index;
                break;
            }
        }
        for (int index = 0; index < static_cast<int>(lines.size()); ++index) {
            const double timestamp = lines[static_cast<std::size_t>(index)].timestamp_seconds;
            if (timestamp < 0.0) {
                continue;
            }
            if (timestamp <= elapsed_seconds) {
                active = index;
            } else {
                break;
            }
        }
        return active;
    }
    const double progress = std::clamp(elapsed_seconds / std::max(1, duration_seconds), 0.0, 0.999);
    return std::clamp(static_cast<int>(progress * static_cast<double>(lines.size())), 0, static_cast<int>(lines.size()) - 1);
}

} // namespace lofibox::ui::widgets
