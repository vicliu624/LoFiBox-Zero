// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "platform/surface_presenter.h"

namespace lofibox::platform::device {

class LinuxFramebufferPlatform : public SurfacePresenter {
public:
    LinuxFramebufferPlatform(std::string framebuffer_path, std::string input_device_path);
    ~LinuxFramebufferPlatform() override;

    [[nodiscard]] bool pump() override;
    [[nodiscard]] std::vector<app::InputEvent> drainInput() override;
    void present(const core::Canvas& canvas) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_{};
};

} // namespace lofibox::platform::device
