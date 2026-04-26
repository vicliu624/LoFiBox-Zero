// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>

#include "platform/surface_presenter.h"

namespace lofibox::app {

void runHelloApp(
    platform::SurfacePresenter& presenter,
    std::chrono::milliseconds auto_exit_after = std::chrono::milliseconds::zero());

} // namespace lofibox::app
