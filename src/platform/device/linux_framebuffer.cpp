// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/device/linux_framebuffer.h"

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <utility>

#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace lofibox::platform::device {
namespace {

std::runtime_error makeSystemError(const std::string& step)
{
    return std::runtime_error(step + ": " + std::strerror(errno));
}

} // namespace

struct LinuxFramebuffer::Impl {
    explicit Impl(std::string framebuffer_path)
    {
        fd = ::open(framebuffer_path.c_str(), O_RDWR);
        if (fd < 0) {
            throw makeSystemError("open framebuffer");
        }

        if (::ioctl(fd, FBIOGET_FSCREENINFO, &fixed_info) < 0) {
            throw makeSystemError("FBIOGET_FSCREENINFO");
        }

        if (::ioctl(fd, FBIOGET_VSCREENINFO, &variable_info) < 0) {
            throw makeSystemError("FBIOGET_VSCREENINFO");
        }

        framebuffer_size = static_cast<std::size_t>(fixed_info.smem_len);
        auto* mapped = ::mmap(nullptr, framebuffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (mapped == MAP_FAILED) {
            throw makeSystemError("mmap framebuffer");
        }
        framebuffer_memory = reinterpret_cast<std::byte*>(mapped);
    }

    ~Impl()
    {
        if (framebuffer_memory != nullptr && framebuffer_size > 0U) {
            ::munmap(framebuffer_memory, framebuffer_size);
        }
        if (fd >= 0) {
            ::close(fd);
        }
    }

    int fd{-1};
    fb_fix_screeninfo fixed_info{};
    fb_var_screeninfo variable_info{};
    std::size_t framebuffer_size{};
    std::byte* framebuffer_memory{};
};

LinuxFramebuffer::LinuxFramebuffer(std::string framebuffer_path)
    : impl_(std::make_unique<Impl>(std::move(framebuffer_path)))
{
}

LinuxFramebuffer::~LinuxFramebuffer() = default;

const fb_fix_screeninfo& LinuxFramebuffer::fixedInfo() const noexcept
{
    return impl_->fixed_info;
}

const fb_var_screeninfo& LinuxFramebuffer::variableInfo() const noexcept
{
    return impl_->variable_info;
}

std::byte* LinuxFramebuffer::framebufferMemory() const noexcept
{
    return impl_->framebuffer_memory;
}

std::size_t LinuxFramebuffer::framebufferSize() const noexcept
{
    return impl_->framebuffer_size;
}

void LinuxFramebuffer::clear() const noexcept
{
    std::memset(impl_->framebuffer_memory, 0, impl_->framebuffer_size);
}

} // namespace lofibox::platform::device
