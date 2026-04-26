// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "core/canvas.h"

namespace lofibox::ui {

[[nodiscard]] std::string upperText(std::string_view text);
[[nodiscard]] std::string fitText(std::string_view text, std::size_t max_chars);
[[nodiscard]] std::string fitUpper(std::string_view text, std::size_t max_chars);
[[nodiscard]] int centeredX(std::string_view text, int scale) noexcept;

void drawText(core::Canvas& canvas, std::string_view text, int x, int y, core::Color color, int scale = 1);
void drawTopBar(core::Canvas& canvas, std::string_view title, bool show_back, std::string_view left_hint = {}) noexcept;
void drawListPageFrame(core::Canvas& canvas);
void drawGlassListFocus(core::Canvas& canvas, int x, int y, int width, int height);
void drawGlassScrollbar(core::Canvas& canvas, int x, int y, int width, int height, int thumb_y, int thumb_height);
void drawFloatingProgressBar(core::Canvas& canvas, int x, int y, int width, int filled_width);
void drawSoftLyricFocus(core::Canvas& canvas, int x, int center_y, int width, int height);
void drawPaginationDots(core::Canvas& canvas, int x, int y, int count, int selected);
void drawLine(core::Canvas& canvas, int x0, int y0, int x1, int y1, core::Color color, int thickness = 1);
void blitScaledCanvas(
    core::Canvas& target,
    const core::Canvas& source,
    int dst_x,
    int dst_y,
    int dst_w,
    int dst_h,
    std::uint8_t opacity = 255);
[[nodiscard]] core::Color mixColor(core::Color a, core::Color b, float t) noexcept;

} // namespace lofibox::ui
