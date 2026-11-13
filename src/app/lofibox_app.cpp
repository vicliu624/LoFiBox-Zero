// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/lofibox_app.h"

#include <utility>
#include <iostream>

#include "app/app_runtime_context.h"

namespace lofibox::app {

LoFiBoxApp::LoFiBoxApp(
    std::vector<std::filesystem::path> media_roots,
    ui::UiAssets assets,
    RuntimeServices services,
    std::vector<std::string> startup_uris,
    bool enable_external_runtime_transport,
    AppStarter on_start
    )
    : services_(withNullRuntimeServices(std::move(services))),
      app_host_(services_),
      runtime_host_(app_host_.registry()),
      context_(std::make_unique<AppRuntimeContext>(
          std::move(media_roots),
          std::move(assets),
          services_.ui.theme ? *services_.ui.theme : ui::defaultTheme(),
          app_host_,
          runtime_host_.client(),
          std::move(startup_uris)))
{
    runtime_host_.resetEq();
    const auto preferences = services_.ui.preferences;
    const auto revision = preferences->revision();
    bool external_running = false;
    if (enable_external_runtime_transport && preferences->effective(RuntimeFeature::ExternalControl)) {
        external_running = runtime_host_.startExternalTransport();
    }

    if (on_start) {
        on_start(runtime_host_.client(), app_host_.registry());
        subsystems_ = std::move(on_start);
    }
    if (enable_external_runtime_transport || subsystems_) {
        auxiliary_worker_ = std::jthread([this, preferences, revision, external_running,
                                        enable_external_runtime_transport](std::stop_token stop) mutable {
            auto previous = revision;
            while (preferences->waitForChange(previous, stop)) {
                if (stop.stop_requested()) break;
                try {
                    const bool wanted = enable_external_runtime_transport
                        && preferences->effective(RuntimeFeature::ExternalControl);
                    if (wanted != external_running) {
                        if (wanted) external_running = runtime_host_.startExternalTransport();
                        else { runtime_host_.stopExternalTransport(); external_running = false; }
                    }
                    // Target callbacks are idempotent and own their resources.
                    // This thread never holds the GUI/application state mutex.
                    if (subsystems_) subsystems_(runtime_host_.client(), app_host_.registry());
                } catch (const std::exception& error) {
                    std::cerr << "Optional interface update failed: " << error.what() << '\n';
                }
            }
        });
    }
}

LoFiBoxApp::~LoFiBoxApp() = default;

void LoFiBoxApp::update()
{
    const std::lock_guard lock{app_host_.registry().stateMutex()};
    const auto now = clock::now();
    const double delta_seconds = std::chrono::duration<double>(now - last_runtime_tick_).count();
    last_runtime_tick_ = now;
    runtime_host_.tick(delta_seconds);
    context_->update();
}

void LoFiBoxApp::handleInput(const InputEvent& event)
{
    const std::lock_guard lock{app_host_.registry().stateMutex()};
    context_->handleInput(event);
}

void LoFiBoxApp::render(core::Canvas& canvas) const
{
    const std::lock_guard lock{app_host_.registry().stateMutex()};
    context_->render(canvas);
}

AppDebugSnapshot LoFiBoxApp::snapshot() const
{
    const std::lock_guard lock{app_host_.registry().stateMutex()};
    return context_->snapshot();
}

bool LoFiBoxApp::runtimeShutdownRequested() const noexcept
{
    return runtime_host_.shutdownRequested();
}

std::chrono::seconds LoFiBoxApp::screenTimeout() const noexcept
{
    return runtime_host_.screenTimeout();
}

bool LoFiBoxApp::batterySaverEnabled() const noexcept
{
    return services_.ui.preferences->effective(RuntimeFeature::BatterySaver);
}

} // namespace lofibox::app
