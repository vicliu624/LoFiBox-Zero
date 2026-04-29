// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/widgets/tui_box.h"

#include "tui/widgets/tui_text.h"

namespace lofibox::tui::widgets {

BoxGlyphs boxGlyphs(TuiCharset charset)
{
    if (charset == TuiCharset::Ascii || charset == TuiCharset::Minimal) {
        return BoxGlyphs{"+", "+", "+", "+", "-", "|"};
    }
    return {};
}

void drawBox(TuiFrame& frame, int row, int col, int width, int height, TuiCharset charset, std::string_view title)
{
    if (width < 2 || height < 2) {
        return;
    }
    const auto glyphs = boxGlyphs(charset);
    frame.put(row, col, glyphs.top_left);
    frame.put(row, col + width - 1, glyphs.top_right);
    frame.put(row + height - 1, col, glyphs.bottom_left);
    frame.put(row + height - 1, col + width - 1, glyphs.bottom_right);
    for (int x = col + 1; x < col + width - 1; ++x) {
        frame.put(row, x, glyphs.horizontal);
        frame.put(row + height - 1, x, glyphs.horizontal);
    }
    for (int y = row + 1; y < row + height - 1; ++y) {
        frame.put(y, col, glyphs.vertical);
        frame.put(y, col + width - 1, glyphs.vertical);
    }
    if (!title.empty() && width > 4) {
        frame.write(row, col + 2, fitText(std::string{title}, width - 4));
    }
}

} // namespace lofibox::tui::widgets

