// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/pages/now_playing_page.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>

#include "ui/ui_primitives.h"
#include "core/spectrum_display.h"

namespace lofibox::ui::pages {
namespace {

std::string formatDuration(int seconds)
{
    const int minutes = std::max(seconds, 0) / 60;
    const int remainder = std::max(seconds, 0) % 60;

    char buffer[16]{};
    std::snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, remainder);
    return buffer;
}

[[nodiscard]] core::Color spectrumHue(float band_position) noexcept
{
    constexpr auto low = core::rgba(56, 202, 255);
    constexpr auto mid = core::rgba(178, 78, 255);
    constexpr auto high = core::rgba(255, 177, 68);

    if (band_position < 0.55f) {
        return ::lofibox::ui::mixColor(low, mid, band_position / 0.55f);
    }
    return ::lofibox::ui::mixColor(mid, high, (band_position - 0.55f) / 0.45f);
}

[[nodiscard]] float spectrumEnergyForColumn(const std::array<float, 10>& bands, int column, int column_count) noexcept
{
    const float band_position = static_cast<float>(column) / static_cast<float>(std::max(1, column_count - 1));
    const float scaled = band_position * static_cast<float>(bands.size() - 1);
    const int lower = std::clamp(static_cast<int>(std::floor(scaled)), 0, static_cast<int>(bands.size()) - 1);
    const int upper = std::clamp(lower + 1, 0, static_cast<int>(bands.size()) - 1);
    const float blend = scaled - static_cast<float>(lower);
    const auto level = [&](int index) {
        const float value = bands[static_cast<std::size_t>(index)];
        return std::isfinite(value) ? std::clamp(value, 0.0f, 1.0f) : 0.0f;
    };
    return level(lower) * (1.0f - blend) + level(upper) * blend;
}

void renderTidalSpectrum(core::Canvas& canvas, const NowPlayingView& view, const UiTheme& theme)
{
    if (!view.visualization.available || view.status != NowPlayingStatus::Playing) return;

    constexpr int kLeft = 10;
    constexpr int kRight = 310;
    constexpr int kBaseline = core::kSpectrumBaseline;
    constexpr int kMaxHeight = core::kSpectrumMaxHeight;
    constexpr int kColumnWidth = 6;
    constexpr int kGap = 4;
    constexpr int kStride = kColumnWidth + kGap;
    constexpr int kColumnCount = (kRight - kLeft) / kStride;

    for (int column = 0; column < kColumnCount; ++column) {
        const float position = static_cast<float>(column) / static_cast<float>(std::max(1, kColumnCount - 1));
        const float energy = spectrumEnergyForColumn(view.visualization.bands, column, kColumnCount);
        const int height = core::spectrumBarHeight(energy);
        const int x = kLeft + (column * kStride);
        const auto base_color = spectrumHue(position);

        for (int row = 0; row < height; ++row) {
            const int y = kBaseline - row;
            const float vertical = static_cast<float>(row) / static_cast<float>(kMaxHeight - 1);
            const float tip = std::min(1.0f, static_cast<float>(height - row) / 5.0f);
            const float opacity = (0.25f - 0.15f * vertical) * tip;
            // This layer is drawn over the uniform panel before any foreground.
            // Blend once per scanline, not per pixel; no overlay canvas or blur.
            canvas.fillRect(x, y, kColumnWidth, 1,
                ::lofibox::ui::mixColor(theme.palette.panel0, base_color, opacity * 0.55f));
            canvas.fillRect(x + 1, y, kColumnWidth - 2, 1,
                ::lofibox::ui::mixColor(theme.palette.panel0, base_color, opacity));
        }
    }
}

} // namespace

void renderNowPlayingPage(core::Canvas& canvas, const NowPlayingView& view, const UiTheme& theme)
{
    if (!view.has_track) {
        ::lofibox::ui::drawText(canvas, "NO TRACK", 122, 38, theme.palette.text_primary, 1);
        ::lofibox::ui::drawText(canvas, "SELECT MUSIC TO PLAY", 122, 58, theme.palette.text_muted, 1);
        return;
    }

    renderTidalSpectrum(canvas, view, theme);

    constexpr int kArtworkFrameX = 32;
    constexpr int kArtworkFrameY = 46;
    constexpr int kArtworkFrameSize = 72;
    constexpr int kArtworkInset = 2;
    constexpr int kArtworkSize = kArtworkFrameSize - (kArtworkInset * 2);

    if (view.artwork != nullptr) {
        ::lofibox::ui::blitScaledCanvas(
            canvas,
            *view.artwork,
            kArtworkFrameX + kArtworkInset,
            kArtworkFrameY + kArtworkInset,
            kArtworkSize,
            kArtworkSize,
            255);
        canvas.strokeRect(kArtworkFrameX, kArtworkFrameY, kArtworkFrameSize, kArtworkFrameSize, theme.palette.divider, 1);
    } else {
        canvas.fillRect(kArtworkFrameX, kArtworkFrameY, kArtworkFrameSize, kArtworkFrameSize, theme.palette.panel1);
        canvas.strokeRect(kArtworkFrameX, kArtworkFrameY, kArtworkFrameSize, kArtworkFrameSize, theme.palette.divider, 1);
        canvas.fillRect(kArtworkFrameX + 18, kArtworkFrameY + 28, 16, 34, theme.palette.progress_strong);
        canvas.fillRect(kArtworkFrameX + 38, kArtworkFrameY + 18, 18, 44, theme.palette.progress);
        ::lofibox::ui::drawLine(canvas, kArtworkFrameX - 2, kArtworkFrameY + 8, kArtworkFrameX + 20, kArtworkFrameY - 2, theme.palette.focus_edge);
        ::lofibox::ui::drawLine(canvas, kArtworkFrameX - 2, kArtworkFrameY + 42, kArtworkFrameX + 20, kArtworkFrameY + 42, theme.palette.focus_edge);
        ::lofibox::ui::drawText(canvas, "NO ART", kArtworkFrameX + 12, kArtworkFrameY + 58, theme.palette.text_muted, 1);
    }

    ::lofibox::ui::drawText(canvas, view.title, 116, 30, theme.palette.text_primary, 1);
    ::lofibox::ui::drawText(canvas, view.artist, 116, 52, theme.palette.text_secondary, 1);
    ::lofibox::ui::drawText(canvas, view.album, 116, 68, theme.palette.text_muted, 1);

    ::lofibox::ui::drawFloatingProgressBar(
        canvas,
        theme,
        116,
        92,
        184,
        std::max(0, std::min(184, static_cast<int>((view.elapsed_seconds / std::max(1, view.duration_seconds)) * 184.0))));

    ::lofibox::ui::drawText(canvas, formatDuration(static_cast<int>(view.elapsed_seconds)), 116, 102, theme.palette.text_secondary, 1);
    ::lofibox::ui::drawText(canvas, formatDuration(view.duration_seconds), 262, 102, theme.palette.text_secondary, 1);

    const auto mode = view.repeat_one ? "ONE" : (view.repeat_all ? "REPEAT" : (view.shuffle_enabled ? "SHUFFLE" : "ORDER"));
    ::lofibox::ui::drawText(canvas, mode, 32, 124, theme.palette.text_secondary, 1);

    if (view.effect_name != "OFF") {
        const auto remix_label = ::lofibox::ui::fitUpper(view.effect_name, 7);
        constexpr int kWidth = 320;
        const int remix_x = kWidth - static_cast<int>(remix_label.size()) * 7 - 10;
        ::lofibox::ui::drawText(canvas, remix_label, remix_x, 4, theme.palette.focus_edge, 1);
    }

}

} // namespace lofibox::ui::pages
