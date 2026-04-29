// SPDX-License-Identifier: GPL-3.0-or-later

#include "tui/tui_layout_engine.h"

namespace lofibox::tui {

TuiLayout classifyTuiLayout(TerminalSize size, const TuiOptions& options)
{
    TuiLayout layout{};
    layout.size = size;
    if (options.layout_override) {
        layout.kind = *options.layout_override;
    } else if (size.width < 32 || size.height < 8) {
        layout.kind = TuiLayoutKind::TooSmall;
    } else if (size.width < 40 || size.height < 10) {
        layout.kind = TuiLayoutKind::Tiny;
    } else if (size.width < 60 || size.height < 16) {
        layout.kind = TuiLayoutKind::Micro;
    } else if (size.width < 80 || size.height < 24) {
        layout.kind = TuiLayoutKind::Compact;
    } else if (size.width < 100 || size.height < 30) {
        layout.kind = TuiLayoutKind::Normal;
    } else {
        layout.kind = TuiLayoutKind::Wide;
    }

    layout.show_header = layout.kind != TuiLayoutKind::Tiny;
    layout.show_spectrum = layout.kind != TuiLayoutKind::TooSmall;
    layout.show_lyrics = layout.kind != TuiLayoutKind::Tiny;
    layout.show_queue = layout.kind == TuiLayoutKind::Compact
        || layout.kind == TuiLayoutKind::Normal
        || layout.kind == TuiLayoutKind::Wide;
    layout.show_footer = layout.kind != TuiLayoutKind::TooSmall;
    return layout;
}

} // namespace lofibox::tui

