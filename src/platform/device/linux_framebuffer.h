// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <memory>
#include <string>

struct fb_fix_screeninfo;
struct fb_var_screeninfo;

namespace lofibox::platform::device {

class LinuxFramebuffer {
public:
    explicit LinuxFramebuffer(std::string framebuffer_path);
    ~LinuxFramebuffer();

    LinuxFramebuffer(const LinuxFramebuffer&) = delete;
    LinuxFramebuffer& operator=(const LinuxFramebuffer&) = delete;

    [[nodiscard]] const fb_fix_screeninfo& fixedInfo() const noexcept;
    [[nodiscard]] const fb_var_screeninfo& variableInfo() const noexcept;
    [[nodiscard]] std::byte* framebufferMemory() const noexcept;
    [[nodiscard]] std::size_t framebufferSize() const noexcept;
    void clear() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_{};
};

} // namespace lofibox::platform::device
