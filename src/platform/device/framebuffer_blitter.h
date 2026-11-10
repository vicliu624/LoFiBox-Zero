// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>
#include <linux/fb.h>
#include "core/canvas.h"

namespace lofibox::platform::device {

// Owns presentation history, not the mapping. The active application must have
// exclusive use of the visible framebuffer while this object is alive.
class FramebufferBlitter {
public:
    FramebufferBlitter(fb_fix_screeninfo fixed, fb_var_screeninfo variable,
                       std::span<std::byte> memory);
    // Returns bytes submitted to the mapping (zero for an unchanged canvas).
    std::size_t present(const core::Canvas& canvas);

private:
    fb_fix_screeninfo fixed_{};
    fb_var_screeninfo variable_{};
    std::span<std::byte> memory_;
    std::array<std::array<std::uint32_t, 256>, 4> channels_{};
    std::vector<core::Color> previous_;
    std::vector<std::byte> row_;
    int width_{};
    int height_{};
    int bytes_per_pixel_{};
};

} // namespace lofibox::platform::device
