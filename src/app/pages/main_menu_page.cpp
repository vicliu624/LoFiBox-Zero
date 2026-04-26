// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/pages/main_menu_page.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>

#include "app/ui/ui_primitives.h"
#include "app/ui/ui_theme.h"
#include "core/bitmap_font.h"
#include "core/display_profile.h"

namespace lofibox::app::pages {
namespace {

using core::rgba;

struct MenuCardTheme {
    core::Color glow{};
    core::Color border{};
    core::Color top{};
    core::Color bottom{};
    core::Color shine{};
    core::Color label{};
    core::Color icon_primary{};
    core::Color icon_secondary{};
    core::Color icon_tertiary{};
};

[[nodiscard]] core::Color alphaBlend(core::Color dst, core::Color src, std::uint8_t opacity) noexcept
{
    const float src_alpha = (static_cast<float>(src.a) / 255.0f) * (static_cast<float>(opacity) / 255.0f);
    const float dst_alpha = static_cast<float>(dst.a) / 255.0f;
    const float out_alpha = src_alpha + (dst_alpha * (1.0f - src_alpha));
    if (out_alpha <= 0.0f) {
        return rgba(0, 0, 0, 0);
    }

    const auto blend = [&](std::uint8_t dst_c, std::uint8_t src_c) -> std::uint8_t {
        const float out =
            ((static_cast<float>(src_c) * src_alpha) + (static_cast<float>(dst_c) * dst_alpha * (1.0f - src_alpha))) / out_alpha;
        return static_cast<std::uint8_t>(std::clamp(out, 0.0f, 255.0f));
    };

    return rgba(
        blend(dst.r, src.r),
        blend(dst.g, src.g),
        blend(dst.b, src.b),
        static_cast<std::uint8_t>(std::clamp(out_alpha * 255.0f, 0.0f, 255.0f)));
}

void blendPixel(core::Canvas& canvas, int x, int y, core::Color color, std::uint8_t opacity)
{
    canvas.setPixel(x, y, alphaBlend(canvas.pixel(x, y), color, opacity));
}

void drawFadedTextWindow(core::Canvas& canvas, std::string_view text, int x, int y, int width, int scroll_px, core::Color color)
{
    if (width <= 0 || text.empty()) {
        return;
    }

    constexpr int kScale = 1;
    constexpr int kFadeWidth = 18;
    const int text_width = core::bitmap_font::measureText(text, kScale);
    const int text_height = core::bitmap_font::lineHeight(kScale);
    if (text_width <= 0 || text_height <= 0) {
        return;
    }

    const int source_width = std::max(width, text_width + width + 32);
    core::Canvas source{source_width, text_height + 2};
    source.clear(rgba(0, 0, 0, 0));

    if (text_width <= width - 4) {
        ui::drawText(source, text, source_width - text_width, 0, color, kScale);
    } else {
        const int cycle_width = text_width + 36;
        const int offset = scroll_px % cycle_width;
        ui::drawText(source, text, -offset, 0, color, kScale);
        ui::drawText(source, text, -offset + cycle_width, 0, color, kScale);
    }

    for (int row = 0; row < source.height(); ++row) {
        for (int col = 0; col < width; ++col) {
            const auto pixel = source.pixel(col, row);
            if (pixel.a == 0) {
                continue;
            }

            const float left_fade = std::clamp(static_cast<float>(col) / static_cast<float>(kFadeWidth), 0.0f, 1.0f);
            const float right_fade = std::clamp(static_cast<float>(width - 1 - col) / static_cast<float>(kFadeWidth), 0.0f, 1.0f);
            const float edge_fade = std::min(left_fade, right_fade);
            const auto opacity = static_cast<std::uint8_t>(std::clamp(edge_fade * 220.0f, 0.0f, 220.0f));
            if (opacity > 0) {
                blendPixel(canvas, x + col, y + row, pixel, opacity);
            }
        }
    }
}

void drawMainMenuTopbar(core::Canvas& canvas, const MainMenuView& view)
{
    canvas.fillRect(0, 0, core::kDisplayWidth, 20, ui::kBgRoot);
    canvas.fillRect(0, 0, core::kDisplayWidth, 14, ui::kChromeTopbar0);
    canvas.fillRect(0, 13, core::kDisplayWidth, 7, ui::kChromeTopbar1);
    canvas.fillRect(0, 19, core::kDisplayWidth, 1, ui::kDivider);

    ui::drawText(canvas, "F1:HELP", 6, 6, ui::kTextSecondary, 1);

    constexpr int kInfoX = 118;
    constexpr int kInfoY = 6;
    constexpr int kInfoWidth = core::kDisplayWidth - kInfoX - 8;
    const auto text_color = view.playback_summary == "NO TRACK" ? ui::kTextMuted : ui::kTextPrimary;
    drawFadedTextWindow(canvas, view.playback_summary, kInfoX, kInfoY, kInfoWidth, view.playback_summary_scroll_px, text_color);
}

void drawMusicIcon(core::Canvas& canvas, int x, int y, int width, int height, const MenuCardTheme& theme)
{
    const int stem_x = x + width / 2 - 2;
    const int stem_y = y + 14;
    ui::drawLine(canvas, stem_x, stem_y, stem_x, y + height - 24, theme.icon_primary, 3);
    ui::drawLine(canvas, stem_x, stem_y, x + width - 20, y + 8, theme.icon_primary, 3);
    ui::drawLine(canvas, x + width - 18, y + 8, x + width - 18, y + height - 30, theme.icon_primary, 3);
    canvas.fillRoundedRect(x + 12, y + height - 30, 20, 20, 10, theme.icon_primary);
    canvas.fillRoundedRect(x + width - 30, y + height - 36, 20, 20, 10, theme.icon_primary);
}

void drawFolderIcon(core::Canvas& canvas, int x, int y, int width, int height, const MenuCardTheme& theme)
{
    canvas.fillRoundedRect(x + 8, y + 18, width - 18, height - 30, 6, theme.icon_secondary);
    canvas.fillRoundedRect(x + 14, y + 10, width / 2, 18, 6, theme.icon_primary);
    canvas.fillRoundedRect(x + 8, y + 22, width - 18, height - 32, 6, theme.icon_primary);
    canvas.strokeRect(x + 8, y + 22, width - 18, height - 32, ui::mixColor(theme.icon_primary, ui::kTextPrimary, 0.25f), 1);
}

void drawPlaylistsIcon(core::Canvas& canvas, int x, int y, int width, int height, const MenuCardTheme& theme)
{
    canvas.fillRoundedRect(x + 8, y + 20, width - 24, height - 34, 6, theme.icon_secondary);
    canvas.fillRoundedRect(x + 16, y + 10, width - 24, height - 34, 6, theme.icon_tertiary);
    canvas.fillRoundedRect(x + 26, y + 20, width - 26, height - 30, 6, theme.icon_primary);
    canvas.fillRect(x + 36, y + 32, width - 46, 4, ui::mixColor(ui::kTextPrimary, theme.icon_primary, 0.4f));
    canvas.fillRect(x + 36, y + 44, width - 50, 4, ui::mixColor(ui::kTextPrimary, theme.icon_primary, 0.4f));
    canvas.fillRect(x + 36, y + 56, width - 54, 4, ui::mixColor(ui::kTextPrimary, theme.icon_primary, 0.4f));
}

void drawMenuCard(
    core::Canvas& canvas,
    int x,
    int y,
    int width,
    int height,
    std::string_view label,
    const MenuCardTheme& theme,
    std::string_view kind,
    const core::Canvas* icon_canvas)
{
    if (icon_canvas != nullptr) {
        ui::blitScaledCanvas(canvas, *icon_canvas, x, y, width, height, 255);
    } else if (kind == "MUSIC") {
        drawMusicIcon(canvas, x, y, width, height, theme);
    } else if (kind == "LIBRARY") {
        drawFolderIcon(canvas, x, y, width, height, theme);
    } else if (kind == "PLAYLISTS") {
        drawPlaylistsIcon(canvas, x, y, width, height, theme);
    } else {
        canvas.fillRoundedRect(x + 8, y + 10, width - 16, height - 20, 6, theme.icon_primary);
        ui::drawText(canvas, ui::fitUpper(label, 8), x + 12, y + height / 2 - 4, ui::kTextPrimary, 1);
    }
}

} // namespace

void renderMainMenuPage(core::Canvas& canvas, const MainMenuView& view)
{
    constexpr std::array<std::string_view, 6> kItems{"MUSIC", "LIBRARY", "PLAYLISTS", "NOW", "EQ", "SETTINGS"};
    constexpr int kCenterX = 112;
    constexpr int kSideY = 38;
    constexpr int kCenterY = 28;
    constexpr int kSideSize = 68;
    constexpr int kCenterSize = 96;

    const auto theme_for = [](int index) {
        constexpr std::array<MenuCardTheme, 6> kThemes{
            MenuCardTheme{rgba(87, 78, 255), rgba(80, 143, 255), rgba(64, 168, 255), rgba(32, 52, 198), rgba(191, 238, 255), ui::kTextPrimary, rgba(235, 245, 255), rgba(96, 174, 255), rgba(54, 69, 225)},
            MenuCardTheme{rgba(0, 214, 255), rgba(89, 216, 255), rgba(96, 229, 246), rgba(43, 124, 224), rgba(214, 255, 255), ui::kTextPrimary, rgba(80, 209, 228), rgba(64, 171, 223), rgba(90, 238, 255)},
            MenuCardTheme{rgba(255, 75, 126), rgba(255, 205, 80), rgba(255, 120, 155), rgba(97, 40, 222), rgba(255, 238, 165), ui::kTextPrimary, rgba(253, 118, 67), rgba(126, 77, 255), rgba(255, 82, 156)},
            MenuCardTheme{rgba(66, 220, 140), rgba(101, 230, 170), rgba(89, 226, 166), rgba(34, 117, 84), rgba(210, 255, 228), ui::kTextPrimary, rgba(112, 236, 178), rgba(44, 143, 96), rgba(126, 255, 213)},
            MenuCardTheme{rgba(255, 178, 87), rgba(255, 127, 42), rgba(255, 192, 92), rgba(177, 73, 33), rgba(255, 240, 196), ui::kTextPrimary, rgba(255, 178, 87), rgba(255, 127, 42), rgba(255, 222, 127)},
            MenuCardTheme{rgba(172, 124, 255), rgba(143, 101, 255), rgba(172, 124, 255), rgba(78, 55, 173), rgba(226, 215, 255), ui::kTextPrimary, rgba(172, 124, 255), rgba(94, 217, 255), rgba(255, 112, 196)},
        };
        return kThemes[static_cast<std::size_t>(std::clamp(index, 0, static_cast<int>(kThemes.size()) - 1))];
    };

    const auto icon_for = [&](int index) -> const core::Canvas* {
        if (view.assets == nullptr) {
            return nullptr;
        }
        switch (index) {
        case 0: return view.assets->music_icon ? &*view.assets->music_icon : nullptr;
        case 1: return view.assets->library_icon ? &*view.assets->library_icon : nullptr;
        case 2: return view.assets->playlists_icon ? &*view.assets->playlists_icon : nullptr;
        case 3: return view.assets->now_playing_icon ? &*view.assets->now_playing_icon : nullptr;
        case 4: return view.assets->equalizer_icon ? &*view.assets->equalizer_icon : nullptr;
        case 5: return view.assets->settings_icon ? &*view.assets->settings_icon : nullptr;
        default: return nullptr;
        }
    };

    canvas.fillRect(0, 0, core::kDisplayWidth, core::kDisplayHeight, ui::kBgRoot);
    drawMainMenuTopbar(canvas, view);

    const auto wrap_index = [&](int index) {
        const int count = static_cast<int>(kItems.size());
        return ((index % count) + count) % count;
    };
    const int selected = wrap_index(view.selected);
    const int left_index = wrap_index(selected - 1);
    const int right_index = wrap_index(selected + 1);

    drawMenuCard(canvas, 22, kSideY, kSideSize, kSideSize, kItems[static_cast<std::size_t>(left_index)], theme_for(left_index), kItems[static_cast<std::size_t>(left_index)], icon_for(left_index));
    ui::drawText(canvas, ui::fitUpper(kItems[static_cast<std::size_t>(left_index)], 10), 18, 120, ui::kTextSecondary, 1);

    drawMenuCard(canvas, kCenterX, kCenterY, kCenterSize, kCenterSize, kItems[static_cast<std::size_t>(selected)], theme_for(selected), kItems[static_cast<std::size_t>(selected)], icon_for(selected));
    ui::drawText(canvas, kItems[static_cast<std::size_t>(selected)], ui::centeredX(kItems[static_cast<std::size_t>(selected)], 1), 126, ui::kTextPrimary, 1);

    drawMenuCard(canvas, 230, kSideY, kSideSize, kSideSize, kItems[static_cast<std::size_t>(right_index)], theme_for(right_index), kItems[static_cast<std::size_t>(right_index)], icon_for(right_index));
    ui::drawText(canvas, ui::fitUpper(kItems[static_cast<std::size_t>(right_index)], 10), 206, 120, ui::kTextSecondary, 1);

    ui::drawPaginationDots(canvas, 135, 152, static_cast<int>(kItems.size()), selected);
}

} // namespace lofibox::app::pages
