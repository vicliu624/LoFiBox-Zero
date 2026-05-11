// SPDX-License-Identifier: GPL-3.0-or-later

#include "ui/pages/groove/midi_overlay.h"

#include "ui/pages/groove/groove_ui_helpers.h"

namespace lofibox::ui::pages::groove {

void renderMidiOverlay(core::Canvas& canvas, const MidiOverlayView& view, const UiTheme& theme)
{
    drawGrooveFrame(canvas, theme, "MIDI");
    drawGrooveValueRow(canvas, theme, 44, "CLOCK", view.clock, view.selectedRow == 0);
    drawGrooveValueRow(canvas, theme, 64, "IN", view.input, view.selectedRow == 1);
    drawGrooveValueRow(canvas, theme, 84, "OUT", view.output, view.selectedRow == 2);
    drawGrooveValueRow(canvas, theme, 104, "SYNC", view.sync, view.selectedRow == 3);
    drawGrooveValueRow(canvas, theme, 124, "DEVICE", view.device, false);
    drawGrooveFooter(canvas, theme, "OK EDIT   LEFT/RIGHT VALUE   BACK");
}

} // namespace lofibox::ui::pages::groove
