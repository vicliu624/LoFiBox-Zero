// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <vector>

#include "app/input_event.h"
#include "platform/surface_presenter.h"

namespace lofibox::platform::x11 {

class X11Presenter final : public SurfacePresenter {
public:
    X11Presenter();
    ~X11Presenter() override;

    X11Presenter(const X11Presenter&) = delete;
    X11Presenter& operator=(const X11Presenter&) = delete;

    [[nodiscard]] bool pump() override;
    [[nodiscard]] std::vector<app::InputEvent> drainInput() override;
    void present(const core::Canvas& canvas) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_{};
};

} // namespace lofibox::platform::x11
