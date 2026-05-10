// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/app_groove_bridge.h"
#include "core/canvas.h"
#include "ui/pages/groove/capture_overlay.h"
#include "ui/pages/groove/chain_overlay.h"
#include "ui/pages/groove/export_overlay.h"
#include "ui/pages/groove/fx_overlay.h"
#include "ui/pages/groove/midi_overlay.h"
#include "ui/pages/groove/project_overlay.h"
#include "ui/pages/groove/sample_edit_overlay.h"
#include "ui/pages/groove/slice_overlay.h"
#include "ui/ui_theme.h"

#include <cassert>

namespace {

class FakePlaybackControl final : public lofibox::app::GroovePlaybackControl {
public:
    void pauseCurrentPlaybackForGroove() override { paused = true; }
    bool paused{false};
};

[[nodiscard]] int litPixels(const lofibox::core::Canvas& canvas)
{
    int count = 0;
    for (const auto& pixel : canvas.pixels()) {
        if (pixel != lofibox::ui::defaultTheme().palette.background) {
            ++count;
        }
    }
    return count;
}

} // namespace

int main()
{
    auto project = lofibox::groove::makeDefaultGrooveProject("UI");
    project.sounds[0].type = lofibox::groove::GrooveSoundType::BuiltinSample;
    project.sounds[0].name = "KIK";
    project.patterns[0].tracks[0].steps[0].trigger = true;
    project.patterns[0].tracks[0].steps[4].trigger = true;

    lofibox::app::AppGrooveBridge bridge{project};
    FakePlaybackControl playback{};
    bridge.enter(playback);
    assert(playback.paused);
    assert(bridge.active());

    lofibox::core::Canvas canvas{320, 170};
    const auto& theme = lofibox::ui::defaultTheme();
    lofibox::ui::pages::groove::renderPocketGrooveMainView(canvas, bridge.mainView(), theme);
    assert(canvas.width() == 320);
    assert(canvas.height() == 170);
    assert(litPixels(canvas) > 1000);

    lofibox::ui::pages::groove::renderCaptureOverlay(canvas, {}, theme);
    assert(litPixels(canvas) > 1000);
    lofibox::ui::pages::groove::renderSampleEditOverlay(canvas, {}, theme);
    assert(litPixels(canvas) > 1000);
    lofibox::ui::pages::groove::renderSliceOverlay(canvas, {}, theme);
    assert(litPixels(canvas) > 1000);
    lofibox::ui::pages::groove::renderChainOverlay(canvas, {{{"A1", 4}, {"A2", 8}}, 1}, theme);
    assert(litPixels(canvas) > 1000);
    lofibox::ui::pages::groove::renderFxOverlay(canvas, {2}, theme);
    assert(litPixels(canvas) > 1000);
    lofibox::ui::pages::groove::renderMidiOverlay(canvas, {}, theme);
    assert(litPixels(canvas) > 1000);
    lofibox::ui::pages::groove::renderExportOverlay(canvas, {}, theme);
    assert(litPixels(canvas) > 1000);
    lofibox::ui::pages::groove::renderProjectOverlay(canvas, {}, theme);
    assert(litPixels(canvas) > 1000);

    bridge.exit();
    assert(!bridge.active());
    return 0;
}
