// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/pages/groove/chain_overlay.h"

#include <algorithm>

#include "ui/pages/groove/groove_ui_helpers.h"
#include "ui/ui_primitives.h"

namespace lofibox::ui::pages::groove {

void renderChainOverlay(core::Canvas& canvas, const ChainOverlayView& view, const UiTheme& theme)
{
    drawGrooveFrame(canvas, theme, "CHAIN");
    const int visible = std::min<int>(static_cast<int>(view.items.size()), 5);
    for (int index = 0; index < visible; ++index) {
        const auto& item = view.items[static_cast<std::size_t>(index)];
        const int x = 20 + (index * 56);
        const bool selected = index == static_cast<int>(view.selectedItem);
        if (selected) {
            ::lofibox::ui::drawGlassListFocus(canvas, theme, x - 4, 48, 50, 18);
        }
        ::lofibox::ui::drawText(canvas, item.pattern + "x" + (item.repeats < 10 ? "0" : "") + std::to_string(item.repeats), x, 54, selected ? theme.palette.focus_edge : theme.palette.text_primary, 1);
    }
    if (!view.items.empty()) {
        const auto selected = std::min<std::size_t>(view.selectedItem, view.items.size() - 1U);
        drawGrooveValueRow(canvas, theme, 92, "ITEM", std::to_string(selected + 1U), true);
        drawGrooveValueRow(canvas, theme, 110, "PATTERN", view.items[selected].pattern + "  REPEAT " + std::to_string(static_cast<int>(view.items[selected].repeats)), false);
    }
    drawGrooveFooter(canvas, theme, "OK EDIT   PLUS ADD   DEL REMOVE");
}

} // namespace lofibox::ui::pages::groove
