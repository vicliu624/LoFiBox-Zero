// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/pages/list_page.h"

#include <algorithm>

#include "ui/ui_primitives.h"
#include "ui/ui_theme.h"
#include "core/display_profile.h"

namespace lofibox::ui::pages {
namespace {

void drawListRowAt(
    core::Canvas& canvas,
    int top,
    int row_index,
    std::string_view primary,
    std::string_view secondary,
    bool selected)
{
    const int y = top + (row_index * ::lofibox::ui::kListRowHeight);
    const auto primary_color = selected ? ::lofibox::ui::kTextPrimary : ::lofibox::ui::kTextPrimary;
    const auto secondary_color = selected ? ::lofibox::ui::kTextPrimary : ::lofibox::ui::kTextSecondary;

    canvas.fillRect(::lofibox::ui::kListInset, y, core::kDisplayWidth - (::lofibox::ui::kListInset * 2), ::lofibox::ui::kListRowHeight - 1, ::lofibox::ui::kBgPanel1);
    if (selected) {
        ::lofibox::ui::drawGlassListFocus(
            canvas,
            ::lofibox::ui::kListInset,
            y,
            core::kDisplayWidth - (::lofibox::ui::kListInset * 2),
            ::lofibox::ui::kListRowHeight - 1);
    } else {
        canvas.fillRect(::lofibox::ui::kListInset, y + ::lofibox::ui::kListRowHeight - 2, core::kDisplayWidth - (::lofibox::ui::kListInset * 2), 1, ::lofibox::ui::kDivider);
    }

    ::lofibox::ui::drawText(canvas, ::lofibox::ui::fitUpper(primary, 22), ::lofibox::ui::kListInset + 6, y + 6, primary_color, 1);
    if (!secondary.empty()) {
        ::lofibox::ui::drawText(canvas, ::lofibox::ui::fitUpper(secondary, 10), core::kDisplayWidth - 86, y + 6, secondary_color, 1);
    }
}

void drawEmptyRow(core::Canvas& canvas, std::string_view label)
{
    const int y = ::lofibox::ui::kListTop;
    canvas.fillRect(::lofibox::ui::kListInset, y, core::kDisplayWidth - (::lofibox::ui::kListInset * 2), ::lofibox::ui::kListRowHeight - 1, ::lofibox::ui::kBgPanel1);
    canvas.fillRect(::lofibox::ui::kListInset, y + ::lofibox::ui::kListRowHeight - 2, core::kDisplayWidth - (::lofibox::ui::kListInset * 2), 1, ::lofibox::ui::kDivider);
    ::lofibox::ui::drawText(canvas, label, ::lofibox::ui::kListInset + 6, y + 6, ::lofibox::ui::kTextMuted, 1);
}

void drawListPositionIndicator(core::Canvas& canvas, int selected, int scroll, int item_count)
{
    if (item_count <= ::lofibox::ui::kMaxVisibleRows) {
        return;
    }

    const std::string position = std::to_string(std::clamp(selected + 1, 1, item_count)) + "/" + std::to_string(item_count);
    ::lofibox::ui::drawText(canvas, position, core::kDisplayWidth - 48, 6, ::lofibox::ui::kTextSecondary, 1);

    constexpr int track_x = core::kDisplayWidth - 7;
    constexpr int track_y = ::lofibox::ui::kListTop;
    constexpr int track_h = (::lofibox::ui::kListRowHeight * ::lofibox::ui::kMaxVisibleRows) - 2;
    constexpr int thumb_min_h = 10;

    const int thumb_h = std::max(thumb_min_h, (track_h * ::lofibox::ui::kMaxVisibleRows) / item_count);
    const int max_scroll = std::max(1, item_count - ::lofibox::ui::kMaxVisibleRows);
    const int clamped_scroll = std::clamp(scroll, 0, max_scroll);
    const int thumb_y = track_y + ((track_h - thumb_h) * clamped_scroll / max_scroll);
    ::lofibox::ui::drawGlassScrollbar(canvas, track_x - 2, track_y, 6, track_h, thumb_y, thumb_h);
}

void drawShortcutHelpModal(core::Canvas& canvas)
{
    constexpr int x = 28;
    constexpr int y = 30;
    constexpr int w = core::kDisplayWidth - (x * 2);
    constexpr int h = 112;

    for (int row = 0; row < core::kDisplayHeight; ++row) {
        for (int col = 0; col < core::kDisplayWidth; ++col) {
            const auto pixel = canvas.pixel(col, row);
            canvas.setPixel(col, row, ::lofibox::ui::mixColor(pixel, ::lofibox::ui::kBgRoot, 0.48f));
        }
    }

    canvas.fillRect(x, y, w, h, ::lofibox::ui::kBgPanel1);
    canvas.strokeRect(x, y, w, h, ::lofibox::ui::kFocusEdge, 1);
    ::lofibox::ui::drawGlassListFocus(canvas, x + 1, y + 1, w - 2, 18);
    ::lofibox::ui::drawText(canvas, "SHORTCUTS", ::lofibox::ui::centeredX("SHORTCUTS", 1), y + 7, ::lofibox::ui::kTextPrimary, 1);

    const struct Row {
        std::string_view key;
        std::string_view action;
    } rows[] = {
        {"OK", "OPEN / PLAY"},
        {"PGUP", "PAGE UP"},
        {"PGDN", "PAGE DOWN"},
        {"BACKSPACE", "BACK"},
        {"F2-F8", "PLAYBACK"},
        {"F9", "SEARCH"},
    };

    int text_y = y + 30;
    for (const auto& row : rows) {
        ::lofibox::ui::drawText(canvas, row.key, x + 14, text_y, ::lofibox::ui::kProgress, 1);
        ::lofibox::ui::drawText(canvas, row.action, x + 92, text_y, ::lofibox::ui::kTextPrimary, 1);
        text_y += 14;
    }

}

} // namespace

void renderListPage(core::Canvas& canvas, const ListPageView& view)
{
    ::lofibox::ui::drawListPageFrame(canvas);
    ::lofibox::ui::drawTopBar(canvas, view.title, view.show_back, view.left_hint);

    if (view.rows.empty()) {
        drawEmptyRow(canvas, view.empty_label);
        if (view.help_open) {
            drawShortcutHelpModal(canvas);
        }
        return;
    }

    const int item_count = static_cast<int>(view.rows.size());
    const int start = std::clamp(view.scroll, 0, std::max(0, item_count - ::lofibox::ui::kMaxVisibleRows));
    const int end = std::min(item_count, start + ::lofibox::ui::kMaxVisibleRows);
    for (int index = start; index < end; ++index) {
        drawListRowAt(
            canvas,
            ::lofibox::ui::kListTop,
            index - start,
            view.rows[static_cast<std::size_t>(index)].first,
            view.rows[static_cast<std::size_t>(index)].second,
            index == view.selected);
    }
    drawListPositionIndicator(canvas, view.selected, start, item_count);
    if (view.help_open) {
        drawShortcutHelpModal(canvas);
    }
}

} // namespace lofibox::ui::pages
