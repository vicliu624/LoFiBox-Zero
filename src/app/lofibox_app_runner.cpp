// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/lofibox_app_runner.h"

#include <chrono>
#include <thread>

#include "app/lofibox_app.h"
#include "core/display_profile.h"
#include "core/frame_deadline.h"

namespace lofibox::app {

void runLoFiBoxApp(
    platform::SurfacePresenter& presenter,
    ui::UiAssets assets,
    RuntimeServices services,
    std::chrono::milliseconds auto_exit_after,
    std::vector<std::string> startup_uris,
    AppStarter on_start
)
{
    LoFiBoxApp app{{}, std::move(assets), std::move(services), std::move(startup_uris), true, std::move(on_start)};
    core::Canvas canvas{core::kDisplayWidth, core::kDisplayHeight};

    using clock = std::chrono::steady_clock;
    constexpr auto kFrameTime = std::chrono::milliseconds(33);
    auto next_frame = clock::now();
    const auto started_at = next_frame;
    auto next_render = started_at;
    bool was_render_enabled = false;
    bool was_saver = app.batterySaverEnabled();

    while (presenter.pump()) {
        const auto events = presenter.drainInput();
        for (const auto& event : events) {
            app.handleInput(event);
        }

        if (auto_exit_after > std::chrono::milliseconds::zero() && (clock::now() - started_at) >= auto_exit_after) {
            break;
        }
        if (app.runtimeShutdownRequested()) {
            break;
        }

        app.update();
        if (app.runtimeShutdownRequested()) break;
        presenter.setIdleTimeout(app.screenTimeout());
        const bool render_enabled = presenter.renderEnabled();
        const bool saver = app.batterySaverEnabled();
        // A static saver page needs only progress/status refreshes. Input,
        // wake and mode changes draw immediately at the normal input cadence.
        if (render_enabled && (!saver || !was_render_enabled || saver != was_saver
            || !events.empty() || clock::now() >= next_render)) {
            app.render(canvas);
            presenter.present(canvas);
            next_render = clock::now() + std::chrono::milliseconds{500};
        }
        was_render_enabled = render_enabled;
        was_saver = saver;

        next_frame = core::nextFrameDeadline(next_frame, clock::now(), kFrameTime);
        std::this_thread::sleep_until(next_frame);
    }
}

} // namespace lofibox::app
