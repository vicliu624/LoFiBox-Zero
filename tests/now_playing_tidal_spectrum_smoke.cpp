// SPDX-License-Identifier: GPL-3.0-or-later

#include <array>
#include <cassert>
#include <limits>

#include "ui/pages/now_playing_page.h"
#include "ui/ui_primitives.h"
#include "core/canvas.h"
#include "core/display_profile.h"

int main()
{
    using namespace lofibox;
    const auto& theme = ui::defaultTheme();
    core::Canvas artwork{68, 68};
    artwork.clear(core::rgba(77, 110, 150));
    ui::pages::NowPlayingView view;
    view.has_track = true;
    view.title = "Spectrum Song";
    view.artist = "Artist";
    view.album = "Album";
    view.duration_seconds = 180;
    view.elapsed_seconds = 37.0;
    view.status = ui::pages::NowPlayingStatus::Playing;
    view.artwork = &artwork;

    const auto render = [&](const auto& value) {
        core::Canvas result{core::kDisplayWidth, core::kDisplayHeight};
        ui::drawListPageFrame(result, theme);
        ui::drawTopBar(result, theme, "NOW PLAYING", true);
        ui::pages::renderNowPlayingPage(result, value, theme);
        return result;
    };
    const auto quiet = render(view);
    view.visualization.available = true;
    view.visualization.bands.fill(1.0F);
    const auto full = render(view);
    int tall_background_pixels = 0;
    int cool_pixels = 0;
    int warm_pixels = 0;
    int foreground_pixels = 0;
    for (int y = 0; y < core::kDisplayHeight; ++y) {
        for (int x = 0; x < core::kDisplayWidth; ++x) {
            const auto before = quiet.pixel(x, y);
            const auto after = full.pixel(x, y);
            if (y < 24 || y >= core::kScreenKeyBarTop || x < 10 || x >= 310) {
                assert(before == after); // No intrusion into chrome or action keys.
            }
            if (before == theme.palette.text_primary || before == theme.palette.text_secondary
                || before == theme.palette.text_muted || before == artwork.pixel(0, 0)
                || before == theme.palette.progress_strong) {
                assert(before == after); // Text, artwork and progress stay above the spectrum.
                ++foreground_pixels;
            }
            if (before == theme.palette.panel0 && before != after) {
                // Even maximum energy stays subdued behind the foreground.
                assert(after.r < 80 && after.g < 80 && after.b < 80);
                if (y < 100) ++tall_background_pixels;
                if (after.b > after.r && after.g > after.r) ++cool_pixels;
                if (after.r > after.b && after.r > after.g) ++warm_pixels;
            }
        }
    }
    assert(tall_background_pixels > 6000);
    assert(cool_pixels > 100 && warm_pixels > 100);
    assert(foreground_pixels > 4500);

    assert(full.pixel(11, 130).b > full.pixel(11, 30).b);
    assert(full.pixel(11, 130).b > full.pixel(10, 130).b);
    view.visualization.bands.fill(0.01F);
    assert(render(view).pixel(11, 90) != quiet.pixel(11, 90)); // Quiet music still reaches into the background.
    view.visualization.bands = {0.03F, 0.08F, 0.12F, 0.30F, 0.75F, 0.24F, 0.50F, 0.10F, 0.04F, 0.20F};
    assert(render(view).pixels() != full.pixels());
    view.status = ui::pages::NowPlayingStatus::Paused;
    assert(render(view).pixels() == quiet.pixels());
    view.status = ui::pages::NowPlayingStatus::Playing;
    view.visualization.bands.fill(0.0F);
    assert(render(view).pixels() == quiet.pixels());
    view.visualization.bands.fill(std::numeric_limits<float>::quiet_NaN());
    assert(render(view).pixels() == quiet.pixels());
    view.visualization.bands.fill(-1.0F);
    assert(render(view).pixels() == quiet.pixels());
    view.has_track = false;
    const auto empty = render(view);
    view.visualization.bands.fill(1.0F);
    assert(render(view).pixels() == empty.pixels());
}
