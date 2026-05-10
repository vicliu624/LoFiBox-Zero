// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/pages/groove/pocket_groove_main_view.h"

#include <algorithm>

#include "ui/ui_primitives.h"

namespace lofibox::ui::pages::groove {
namespace {

using core::rgba;

void drawSlot(core::Canvas& canvas, const UiTheme& theme, int index, bool filled, bool selected)
{
    const int row = index < 8 ? 0 : 1;
    const int col = index % 8;
    const int x = 14 + (col * 37);
    const int y = 30 + (row * 16);
    const auto fill = selected ? theme.palette.focus_fill : (filled ? rgba(76, 93, 113) : rgba(18, 22, 28));
    canvas.fillRect(x, y, 29, 11, fill);
    canvas.strokeRect(x, y, 29, 11, selected ? theme.palette.focus_edge : theme.palette.divider, 1);
    const auto label = (index + 1 < 10 ? std::string{"0"} : std::string{}) + std::to_string(index + 1);
    ::lofibox::ui::drawText(canvas, label, x + 8, y + 2, selected ? theme.palette.text_primary : theme.palette.text_muted, 1);
}

void drawTrackRow(core::Canvas& canvas, const UiTheme& theme, const PocketGrooveTrackRow& row, int y)
{
    ::lofibox::ui::drawText(canvas, ::lofibox::ui::fitUpper(row.label, 3), 14, y, theme.palette.text_secondary, 1);
    for (int step = 0; step < 16; ++step) {
        const auto& cell = row.steps[static_cast<std::size_t>(step)];
        const int x = 48 + (step * 15);
        const auto color = cell.trigger ? (cell.locked ? theme.palette.warn : theme.palette.text_primary) : theme.palette.text_muted;
        if (cell.playhead) {
            canvas.fillRect(x - 1, y - 2, 9, 11, rgba(40, 64, 84));
        }
        if (cell.selected) {
            canvas.strokeRect(x - 2, y - 3, 11, 13, theme.palette.focus_edge, 1);
        }
        ::lofibox::ui::drawText(canvas, cell.trigger ? "#" : ".", x, y, color, 1);
    }
}

} // namespace

void renderPocketGrooveMainView(core::Canvas& canvas, const PocketGrooveMainView& view, const UiTheme& theme)
{
    canvas.clear(theme.palette.background);
    const auto title = "GROOVE " + ::lofibox::ui::fitUpper(view.patternName, 3) + " " + std::to_string(view.bpm) + "BPM " + (view.playing ? ">" : "||") + (view.chainEnabled ? " CHAIN" : "");
    ::lofibox::ui::drawTopBar(canvas, theme, title, true);

    canvas.fillRect(6, 24, 308, 140, theme.palette.panel1);
    canvas.strokeRect(6, 24, 308, 140, theme.palette.divider, 1);
    for (int slot = 0; slot < 16; ++slot) {
        drawSlot(canvas, theme, slot, view.filledSlots[static_cast<std::size_t>(slot)], slot == static_cast<int>(view.selectedSlot));
    }

    canvas.fillRect(12, 66, 296, 1, theme.palette.divider);
    for (int row = 0; row < 4; ++row) {
        drawTrackRow(canvas, theme, view.visibleTracks[static_cast<std::size_t>(row)], 74 + (row * 17));
    }

    if (view.heldFx != 0U) {
        canvas.fillRect(248, 54, 54, 11, theme.palette.warn);
        ::lofibox::ui::drawText(canvas, "FX" + std::to_string(static_cast<int>(view.heldFx)), 262, 56, theme.palette.background, 1);
    }

    canvas.fillRect(6, 148, 308, 16, theme.palette.panel2);
    canvas.strokeRect(6, 148, 308, 16, theme.palette.divider, 1);
    ::lofibox::ui::drawText(canvas, ::lofibox::ui::fitUpper(view.footer, 38), 12, 152, theme.palette.text_secondary, 1);
}

} // namespace lofibox::ui::pages::groove
