// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/pages/lyrics_page.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cmath>
#include <cstdint>
#include <optional>
#include <sstream>
#include <vector>

#include "ui/ui_primitives.h"
#include "ui/ui_theme.h"
#include "core/bitmap_font.h"
#include "core/display_profile.h"

namespace lofibox::ui::pages {
namespace {

struct LyricDisplayLine {
    double timestamp_seconds{-1.0};
    std::string text{};
};

[[nodiscard]] core::Color alphaBlend(core::Color dst, core::Color src, std::uint8_t opacity) noexcept
{
    const float src_alpha = (static_cast<float>(src.a) / 255.0f) * (static_cast<float>(opacity) / 255.0f);
    const float dst_alpha = static_cast<float>(dst.a) / 255.0f;
    const float out_alpha = src_alpha + (dst_alpha * (1.0f - src_alpha));
    if (out_alpha <= 0.0f) {
        return core::rgba(0, 0, 0, 0);
    }

    const auto blend = [&](std::uint8_t dst_c, std::uint8_t src_c) -> std::uint8_t {
        const float out =
            ((static_cast<float>(src_c) * src_alpha) + (static_cast<float>(dst_c) * dst_alpha * (1.0f - src_alpha))) / out_alpha;
        return static_cast<std::uint8_t>(std::clamp(out, 0.0f, 255.0f));
    };

    return core::rgba(
        blend(dst.r, src.r),
        blend(dst.g, src.g),
        blend(dst.b, src.b),
        static_cast<std::uint8_t>(std::clamp(out_alpha * 255.0f, 0.0f, 255.0f)));
}

void blendPixel(core::Canvas& canvas, int x, int y, core::Color color, std::uint8_t opacity)
{
    canvas.setPixel(x, y, alphaBlend(canvas.pixel(x, y), color, opacity));
}

[[nodiscard]] core::Color sideFoamHue(float rise_position) noexcept
{
    constexpr auto crest = core::rgba(255, 188, 88);
    constexpr auto mid = core::rgba(178, 78, 255);
    constexpr auto base = core::rgba(56, 202, 255);

    if (rise_position < 0.58f) {
        return ::lofibox::ui::mixColor(base, mid, rise_position / 0.58f);
    }
    return ::lofibox::ui::mixColor(mid, crest, (rise_position - 0.58f) / 0.42f);
}

[[nodiscard]] float sideFoamEnergy(const SpectrumFrame& frame, float rise_position) noexcept
{
    const float band_position = std::clamp(rise_position, 0.0f, 1.0f);
    const float scaled = band_position * static_cast<float>(frame.bands.size() - 1);
    const int lower = std::clamp(static_cast<int>(std::floor(scaled)), 0, static_cast<int>(frame.bands.size()) - 1);
    const int upper = std::clamp(lower + 1, 0, static_cast<int>(frame.bands.size()) - 1);
    const float blend = scaled - static_cast<float>(lower);
    return std::clamp(
        (frame.bands[static_cast<std::size_t>(lower)] * (1.0f - blend)) +
            (frame.bands[static_cast<std::size_t>(upper)] * blend),
        0.0f,
        1.0f);
}

[[nodiscard]] float foamWave(float y, double elapsed_seconds, float side_phase) noexcept
{
    const float time = static_cast<float>(elapsed_seconds);
    const float wave_a = std::sin((y * 0.135f) + (time * 2.20f) + side_phase);
    const float wave_b = std::sin((y * 0.047f) - (time * 1.18f) + (side_phase * 0.71f));
    const float wave_c = std::sin((y * 0.021f) + (time * 0.66f) + (side_phase * 1.43f));
    return (wave_a * 0.48f) + (wave_b * 0.34f) + (wave_c * 0.18f);
}

[[nodiscard]] float adjacentEnergy(const SpectrumFrame& frame, float rise_position) noexcept
{
    constexpr float spread = 0.085f;
    const float lower = sideFoamEnergy(frame, std::clamp(rise_position - spread, 0.0f, 1.0f));
    const float center = sideFoamEnergy(frame, rise_position);
    const float upper = sideFoamEnergy(frame, std::clamp(rise_position + spread, 0.0f, 1.0f));
    return (lower * 0.24f) + (center * 0.52f) + (upper * 0.24f);
}

void renderFoamSide(
    core::Canvas& canvas,
    const SpectrumFrame& frame,
    double elapsed_seconds,
    bool left_side)
{
    constexpr int top = 34;
    constexpr int bottom = 148;
    constexpr int edge_margin = 4;
    constexpr int min_width = 4;
    constexpr int max_width = 34;

    const int edge_x = left_side ? edge_margin : (core::kDisplayWidth - edge_margin - 1);
    const int direction = left_side ? 1 : -1;
    const float side_phase = left_side ? 0.0f : 7.3f;

    for (int y = top; y <= bottom; ++y) {
        const float rise = static_cast<float>(bottom - y) / static_cast<float>(bottom - top);
        const float raw_energy = frame.available ? adjacentEnergy(frame, rise) : 0.0f;
        const float shaped_energy = std::pow(std::clamp(raw_energy, 0.0f, 1.0f), 0.54f);
        const float vertical_fade = 0.30f + (std::pow(1.0f - rise, 0.52f) * 0.70f);
        const float slow_swell = 0.70f + (0.30f * std::sin((elapsed_seconds * 0.72) + (rise * 5.4f) + side_phase));
        const float wave = foamWave(static_cast<float>(y), elapsed_seconds, side_phase);
        const float crest = std::max(0.0f, foamWave(static_cast<float>(y) + 9.0f, elapsed_seconds, side_phase + 2.2f));
        const float width_float =
            static_cast<float>(min_width)
            + (shaped_energy * slow_swell * static_cast<float>(max_width - min_width))
            + (wave * (2.2f + shaped_energy * 5.4f));
        const int reach = std::clamp(static_cast<int>(std::round(width_float)), min_width, max_width);
        const auto color = sideFoamHue(rise);

        for (int offset = 0; offset <= reach; ++offset) {
            const float inward = static_cast<float>(offset) / static_cast<float>(std::max(1, reach));
            const float body = std::pow(1.0f - inward, 1.78f);
            const float boundary = std::exp(-std::pow((inward - 0.82f) * 4.2f, 2.0f));
            const float spray = std::max(
                0.0f,
                static_cast<float>(std::sin((static_cast<float>(y) * 0.31f) + (offset * 0.73f) + (elapsed_seconds * 2.4) + side_phase)));
            const float opacity_float =
                shaped_energy * vertical_fade *
                ((body * 70.0f) + (boundary * crest * 48.0f) + (spray * boundary * 16.0f));
            const auto opacity = static_cast<std::uint8_t>(std::clamp(opacity_float, 0.0f, 116.0f));
            if (opacity == 0) {
                continue;
            }

            const int x = edge_x + (direction * offset);
            blendPixel(canvas, x, y, color, opacity);
            if ((offset % 3) == 0 && y + 1 <= bottom) {
                blendPixel(canvas, x, y + 1, color, static_cast<std::uint8_t>(opacity * 0.42f));
            }
        }
    }
}

void renderSideFoamSpectrum(core::Canvas& canvas, const SpectrumFrame& frame, double elapsed_seconds)
{
    renderFoamSide(canvas, frame, elapsed_seconds, true);
    renderFoamSide(canvas, frame, elapsed_seconds, false);
}

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
    if (minutes_string.empty()
        || seconds_string.empty()
        || minute_end == minutes_string.c_str()
        || *minute_end != '\0'
        || second_end == seconds_string.c_str()
        || *second_end != '\0') {
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

} // namespace

void renderLyricsPage(core::Canvas& canvas, const LyricsPageView& view)
{
    canvas.fillRect(0, ::lofibox::ui::kTopBarHeight, core::kDisplayWidth, core::kDisplayHeight - ::lofibox::ui::kTopBarHeight, ::lofibox::ui::kBgRoot);
    canvas.fillRect(6, 27, 308, 135, ::lofibox::ui::kBgPanel0);
    canvas.fillRect(6, 27, 308, 1, ::lofibox::ui::kDivider);
    renderSideFoamSpectrum(canvas, view.visualization, view.elapsed_seconds);

    if (!view.has_track) {
        ::lofibox::ui::drawText(canvas, "NO TRACK", ::lofibox::ui::centeredX("NO TRACK", 1), 70, ::lofibox::ui::kTextPrimary, 1);
        ::lofibox::ui::drawText(canvas, "SELECT MUSIC TO PLAY", ::lofibox::ui::centeredX("SELECT MUSIC TO PLAY", 1), 92, ::lofibox::ui::kTextMuted, 1);
        return;
    }

    ::lofibox::ui::drawText(canvas, ::lofibox::ui::fitText(view.title, 30), ::lofibox::ui::centeredX(::lofibox::ui::fitText(view.title, 30), 1), 32, ::lofibox::ui::kTextPrimary, 1);
    ::lofibox::ui::drawText(canvas, ::lofibox::ui::fitText(view.artist, 28), ::lofibox::ui::centeredX(::lofibox::ui::fitText(view.artist, 28), 1), 46, ::lofibox::ui::kTextMuted, 1);

    const auto lines = lyricDisplayLines(view.lyrics);
    if (lines.empty()) {
        const std::string primary = view.lookup_pending ? "SEARCHING LYRICS" : "LYRICS NOT FOUND";
        const std::string secondary = view.lookup_pending ? "ONLINE MATCH IN PROGRESS" : "NO MATCH FROM TAGS OR ONLINE";
        canvas.fillRect(54, 72, 212, 1, ::lofibox::ui::kDivider);
        canvas.fillRect(72, 92, 176, 1, ::lofibox::ui::kDivider);
        for (int index = 0; index < 5; ++index) {
            const int x = 122 + (index * 18);
            const int h = 6 + ((index % 3) * 5);
            const auto color = view.lookup_pending && index == static_cast<int>(static_cast<long long>(view.elapsed_seconds) % 5)
                ? ::lofibox::ui::kProgressStrong
                : ::lofibox::ui::kBgPanel2;
            canvas.fillRect(x, 80 - h, 8, h, color);
        }
        ::lofibox::ui::drawText(canvas, primary, ::lofibox::ui::centeredX(primary, 1), 96, view.lookup_pending ? ::lofibox::ui::kTextPrimary : ::lofibox::ui::kTextSecondary, 1);
        ::lofibox::ui::drawText(canvas, secondary, ::lofibox::ui::centeredX(secondary, 1), 114, ::lofibox::ui::kTextMuted, 1);
        ::lofibox::ui::drawText(canvas, "L: BACK TO PLAYER", ::lofibox::ui::centeredX("L: BACK TO PLAYER", 1), 136, ::lofibox::ui::kTextMuted, 1);
    } else {
        constexpr int visible_lines = 7;
        constexpr int row_h = 14;
        constexpr std::size_t active_line_chars = 44;
        constexpr std::size_t inactive_line_chars = 40;
        const int active = activeLyricIndex(lines, view.elapsed_seconds, view.duration_seconds);
        const int first = active - (visible_lines / 2);
        for (int row = 0; row < visible_lines; ++row) {
            const int index = first + row;
            if (index < 0 || index >= static_cast<int>(lines.size())) {
                continue;
            }
            const int y = 62 + (row * row_h);
            const bool is_active = index == active;
            const std::string text = ::lofibox::ui::fitText(
                lines[static_cast<std::size_t>(index)].text,
                is_active ? active_line_chars : inactive_line_chars);
            const int x = ::lofibox::ui::centeredX(text, 1);
            const auto color = is_active ? ::lofibox::ui::kTextPrimary : (std::abs(index - active) <= 1 ? ::lofibox::ui::kTextSecondary : ::lofibox::ui::kTextMuted);
            if (is_active) {
                ::lofibox::ui::drawSoftLyricFocus(canvas, 34, y + 6, 252, 30);
                ::lofibox::ui::drawText(canvas, text, x, y + 2, color, 1);
            } else {
                ::lofibox::ui::drawText(canvas, text, x, y + 2, color, 1);
            }
        }

    }

    ::lofibox::ui::drawFloatingProgressBar(
        canvas,
        55,
        154,
        210,
        std::max(0, std::min(210, static_cast<int>((view.elapsed_seconds / std::max(1, view.duration_seconds)) * 210.0))));
}

} // namespace lofibox::ui::pages
