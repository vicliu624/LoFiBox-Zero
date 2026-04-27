// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/lofibox_app.h"

#include <utility>

#include "app/app_runtime_context.h"

namespace lofibox::app {

LoFiBoxApp::LoFiBoxApp(std::vector<std::filesystem::path> media_roots, ui::UiAssets assets, RuntimeServices services, std::vector<std::string> startup_uris)
    : context_(std::make_unique<AppRuntimeContext>(std::move(media_roots), std::move(assets), std::move(services), std::move(startup_uris)))
{}

LoFiBoxApp::~LoFiBoxApp() = default;

void LoFiBoxApp::update()
{
    context_->update();
}

void LoFiBoxApp::handleInput(const InputEvent& event)
{
    context_->handleInput(event);
}

void LoFiBoxApp::render(core::Canvas& canvas) const
{
    context_->render(canvas);
}

AppDebugSnapshot LoFiBoxApp::snapshot() const
{
    return context_->snapshot();
}

} // namespace lofibox::app
