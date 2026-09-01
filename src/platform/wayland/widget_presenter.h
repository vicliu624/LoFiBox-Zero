// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>

#include "platform/surface_presenter.h"

namespace lofibox::platform::wayland {

// A fixed-size desktop component for compositors that expose
// zwlr_layer_shell_v1. Unlike WaylandPresenter, this is not an xdg_toplevel:
// it never asks the compositor to manage it as a normal application window.
class WidgetPresenter final : public SurfacePresenter {
public:
    WidgetPresenter();
    ~WidgetPresenter() override;

    WidgetPresenter(const WidgetPresenter&) = delete;
    WidgetPresenter& operator=(const WidgetPresenter&) = delete;

    [[nodiscard]] bool pump() override;
    [[nodiscard]] std::vector<app::InputEvent> drainInput() override;
    void present(const core::Canvas& canvas) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_{};
};

} // namespace lofibox::platform::wayland
