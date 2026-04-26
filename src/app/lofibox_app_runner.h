// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>

#include "app/lofibox_app.h"
#include "platform/surface_presenter.h"

namespace lofibox::app {

void runLoFiBoxApp(
    platform::SurfacePresenter& presenter,
    ui::UiAssets assets = {},
    RuntimeServices services = {},
    std::chrono::milliseconds auto_exit_after = std::chrono::milliseconds::zero());

} // namespace lofibox::app
