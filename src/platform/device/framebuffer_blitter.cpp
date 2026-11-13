// SPDX-License-Identifier: GPL-3.0-or-later
#include "platform/device/framebuffer_blitter.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace lofibox::platform::device {

FramebufferBlitter::FramebufferBlitter(fb_fix_screeninfo fixed, fb_var_screeninfo variable,
                                     std::span<std::byte> memory)
    : fixed_(fixed), variable_(variable), memory_(memory)
{
    if (fixed.type != FB_TYPE_PACKED_PIXELS || fixed.visual != FB_VISUAL_TRUECOLOR
        || (variable.bits_per_pixel != 16 && variable.bits_per_pixel != 24 && variable.bits_per_pixel != 32)) {
        throw std::runtime_error("unsupported framebuffer format (expected packed truecolor 16/24/32-bit)");
    }
    bytes_per_pixel_ = static_cast<int>(variable.bits_per_pixel / 8);
    const auto right = static_cast<std::uint64_t>(variable.xoffset) + variable.xres;
    const auto bottom = static_cast<std::uint64_t>(variable.yoffset) + variable.yres;
    if (variable.xres == 0 || variable.yres == 0
        || variable.xres > static_cast<unsigned>(std::numeric_limits<int>::max())
        || variable.yres > static_cast<unsigned>(std::numeric_limits<int>::max())
        || right * bytes_per_pixel_ > fixed.line_length || fixed.line_length == 0
        || bottom > memory.size() / fixed.line_length) {
        throw std::runtime_error("framebuffer viewport exceeds mapped memory or row stride");
    }
    const std::array fields{variable.red, variable.green, variable.blue, variable.transp};
    for (std::size_t channel = 0; channel < fields.size(); ++channel) {
        const auto& field = fields[channel];
        if (field.length > 16 || field.offset > variable.bits_per_pixel
            || field.length > variable.bits_per_pixel - field.offset || field.msb_right != 0) {
            throw std::runtime_error("unsupported framebuffer channel layout");
        }
        const auto maximum = (1U << field.length) - 1U;
        for (std::uint32_t value = 0; value < 256; ++value) {
            channels_[channel][value] = field.length == 0 ? 0U
                : ((value * maximum + 127U) / 255U) << field.offset;
        }
    }
}

std::size_t FramebufferBlitter::present(const core::Canvas& canvas)
{
    if (canvas.width() <= 0 || canvas.height() <= 0) {
        throw std::runtime_error("invalid logical framebuffer dimensions");
    }
    const int physical_width = static_cast<int>(variable_.xres);
    const int physical_height = static_cast<int>(variable_.yres);
    const int scale = std::min(physical_width / canvas.width(), physical_height / canvas.height());
    if (scale == 0) {
        throw std::runtime_error("framebuffer is smaller than the logical display");
    }
    const int offset_x = (physical_width - canvas.width() * scale) / 2;
    const int offset_y = (physical_height - canvas.height() * scale) / 2;
    const bool first = width_ != canvas.width() || height_ != canvas.height();
    std::size_t written = 0;
    if (first) {
        previous_.resize(canvas.pixels().size());
        row_.resize(static_cast<std::size_t>(canvas.width()) * scale * bytes_per_pixel_);
        // Clear only the visible viewport once, preserving virtual pages and padding.
        for (int y = 0; y < physical_height; ++y) {
            const auto position = (static_cast<std::size_t>(variable_.yoffset) + y) * fixed_.line_length
                + static_cast<std::size_t>(variable_.xoffset) * bytes_per_pixel_;
            const auto bytes = static_cast<std::size_t>(physical_width) * bytes_per_pixel_;
            std::memset(memory_.data() + position, 0, bytes);
            written += bytes;
        }
        width_ = canvas.width();
        height_ = canvas.height();
    }
    for (int y = 0; y < height_; ++y) {
        const auto* source = canvas.pixels().data() + static_cast<std::size_t>(y) * width_;
        auto* previous = previous_.data() + static_cast<std::size_t>(y) * width_;
        if (!first && std::memcmp(source, previous, static_cast<std::size_t>(width_) * sizeof(core::Color)) == 0) {
            continue;
        }
        int begin = 0;
        int end = width_;
        if (!first) {
            while (begin < end && source[begin] == previous[begin]) ++begin;
            while (end > begin && source[end - 1] == previous[end - 1]) --end;
        }
        auto* destination = row_.data();
        for (int x = begin; x < end; ++x) {
            const auto color = source[x];
            const auto packed = channels_[0][color.r] | channels_[1][color.g]
                | channels_[2][color.b] | channels_[3][color.a];
            for (int dx = 0; dx < scale; ++dx) {
                for (int byte = 0; byte < bytes_per_pixel_; ++byte) {
                    *destination++ = static_cast<std::byte>((packed >> (byte * 8)) & 0xffU);
                }
            }
        }
        const auto bytes = static_cast<std::size_t>(end - begin) * scale * bytes_per_pixel_;
        for (int dy = 0; dy < scale; ++dy) {
            const auto position = (static_cast<std::size_t>(variable_.yoffset) + offset_y + y * scale + dy)
                * fixed_.line_length
                + (static_cast<std::size_t>(variable_.xoffset) + offset_x + begin * scale) * bytes_per_pixel_;
            std::memcpy(memory_.data() + position, row_.data(), bytes);
            written += bytes;
        }
        std::memcpy(previous + begin, source + begin, static_cast<std::size_t>(end - begin) * sizeof(core::Color));
    }
    return written;
}

} // namespace lofibox::platform::device
