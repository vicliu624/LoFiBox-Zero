// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <vector>
#include <chrono>

#include "app/input_event.h"
#include "core/canvas.h"

namespace lofibox::platform {

class SurfacePresenter {
public:
    virtual ~SurfacePresenter() = default;

    [[nodiscard]] virtual bool pump() = 0;
    [[nodiscard]] virtual std::vector<app::InputEvent> drainInput() = 0;
    virtual void present(const core::Canvas& canvas) = 0;
    virtual void setIdleTimeout(std::chrono::seconds) {}
    [[nodiscard]] virtual bool renderEnabled() const noexcept { return true; }
};

} // namespace lofibox::platform
