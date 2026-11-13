// SPDX-License-Identifier: GPL-3.0-or-later
#include "platform/device/framebuffer_blitter.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using lofibox::core::Canvas;
using lofibox::core::Color;
using lofibox::platform::device::FramebufferBlitter;

fb_var_screeninfo format(int bits, int width, int height)
{
    fb_var_screeninfo info{};
    info.xres = width;
    info.yres = height;
    info.bits_per_pixel = bits;
    info.red = {bits == 16 ? 11U : 16U, bits == 16 ? 5U : 8U, 0};
    info.green = {bits == 16 ? 5U : 8U, bits == 16 ? 6U : 8U, 0};
    info.blue = {0, bits == 16 ? 5U : 8U, 0};
    info.transp = {bits == 32 ? 24U : 0U, bits == 32 ? 8U : 0U, 0};
    return info;
}

std::uint32_t referencePixel(Color color, const fb_var_screeninfo& info)
{
    const auto channel = [](std::uint8_t value, const fb_bitfield& field) {
        return field.length == 0 ? 0U
            : ((value * ((1U << field.length) - 1U) + 127U) / 255U) << field.offset;
    };
    return channel(color.r, info.red) | channel(color.g, info.green)
        | channel(color.b, info.blue) | channel(color.a, info.transp);
}

// The original full-clear/pack/every-pixel path, kept here as an independent
// reference for pixel equivalence and an optional offline microbenchmark.
void referencePresent(const Canvas& canvas, const fb_var_screeninfo& info,
                      const fb_fix_screeninfo& fixed, std::vector<std::byte>& memory)
{
    std::memset(memory.data(), 0, memory.size());
    const int scale = std::min(info.xres / canvas.width(), info.yres / canvas.height());
    const int ox = (info.xres - canvas.width() * scale) / 2;
    const int oy = (info.yres - canvas.height() * scale) / 2;
    const int bpp = info.bits_per_pixel / 8;
    for (int y = 0; y < canvas.height(); ++y) {
        for (int x = 0; x < canvas.width(); ++x) {
            const auto packed = referencePixel(canvas.pixel(x, y), info);
            for (int dy = 0; dy < scale; ++dy) {
                for (int dx = 0; dx < scale; ++dx) {
                    const auto offset = (y * scale + oy + dy + info.yoffset) * fixed.line_length
                        + (x * scale + ox + dx + info.xoffset) * bpp;
                    for (int byte = 0; byte < bpp; ++byte) {
                        memory[offset + byte] = static_cast<std::byte>((packed >> (byte * 8)) & 0xffU);
                    }
                }
            }
        }
    }
}

void expect(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

void verifyFormat(int bits, int scale, bool swap_red_blue)
{
    auto info = format(bits, 7 * scale + 3, 5 * scale + 4);
    if (swap_red_blue) std::swap(info.red, info.blue);
    info.xoffset = 2;
    info.yoffset = 1;
    const int bpp = bits / 8;
    fb_fix_screeninfo fixed{};
    fixed.visual = FB_VISUAL_TRUECOLOR;
    fixed.line_length = (info.xres + info.xoffset) * bpp + 13;
    fixed.smem_len = (info.yres + 3) * fixed.line_length;
    std::vector<std::byte> memory(fixed.smem_len + 32, std::byte{0xa5});
    FramebufferBlitter blitter{fixed, info, std::span{memory}.first(fixed.smem_len)};
    Canvas canvas{7, 5};
    for (int y = 0; y < 5; ++y) {
        for (int x = 0; x < 7; ++x) {
            canvas.setPixel(x, y, Color{static_cast<std::uint8_t>(x * 41),
                static_cast<std::uint8_t>(y * 53), 137, 203});
        }
    }
    expect(blitter.present(canvas) > 0, "first presentation must initialize the viewport");
    const auto verify = [&] {
        std::vector<std::byte> reference(fixed.smem_len);
        referencePresent(canvas, info, fixed, reference);
        for (std::size_t index = 0; index < memory.size(); ++index) {
            const auto row = index / fixed.line_length;
            const auto column = index % fixed.line_length;
            const bool visible = row >= info.yoffset && row < info.yoffset + info.yres
                && column >= info.xoffset * bpp && column < (info.xoffset + info.xres) * bpp;
            expect(memory[index] == (visible ? reference[index] : std::byte{0xa5}),
                "pixel mismatch or overwritten virtual page/padding/guard");
        }
    };
    verify();
    expect(blitter.present(canvas) == 0, "unchanged frame must not touch mapped memory");
    canvas.setPixel(3, 2, Color{255, 0, 0, 255});
    expect(blitter.present(canvas) == static_cast<std::size_t>(scale * scale * bpp),
        "single-pixel change must write only its scaled rectangle");
    verify();
    canvas.setPixel(0, 0, Color{0, 255, 0, 255});
    canvas.setPixel(6, 4, Color{0, 0, 255, 255});
    (void)blitter.present(canvas);
    verify();
    canvas = Canvas{5, 3};
    canvas.clear(Color{11, 22, 33, 255});
    (void)blitter.present(canvas);
    verify();
}

void verifyRejection()
{
    auto info = format(16, 320, 240);
    fb_fix_screeninfo fixed{};
    fixed.visual = FB_VISUAL_TRUECOLOR;
    fixed.line_length = 640;
    std::vector<std::byte> memory(640 * 240);
    const auto reject = [&](const fb_fix_screeninfo& f, const fb_var_screeninfo& v, std::size_t size) {
        bool rejected = false;
        try { FramebufferBlitter invalid{f, v, std::span{memory}.first(size)}; }
        catch (const std::runtime_error&) { rejected = true; }
        expect(rejected, "invalid framebuffer geometry or format was accepted");
    };
    reject(fixed, info, memory.size() - 1);
    auto invalid = info;
    invalid.xoffset = 1;
    reject(fixed, invalid, memory.size());
    invalid = info;
    invalid.yoffset = 1;
    reject(fixed, invalid, memory.size());
    invalid = info;
    invalid.bits_per_pixel = 8;
    reject(fixed, invalid, memory.size());
    invalid = info;
    invalid.red.length = 32;
    reject(fixed, invalid, memory.size());
    invalid = info;
    invalid.xres = 0;
    reject(fixed, invalid, memory.size());
    FramebufferBlitter blitter{fixed, info, memory};
    bool rejected = false;
    try { (void)blitter.present(Canvas{321, 170}); }
    catch (const std::runtime_error&) { rejected = true; }
    expect(rejected, "undersized display must be rejected");
}

void benchmark()
{
    using clock = std::chrono::steady_clock;
    const auto info = format(16, 320, 240);
    fb_fix_screeninfo fixed{};
    fixed.visual = FB_VISUAL_TRUECOLOR;
    fixed.line_length = 640;
    fixed.smem_len = 640 * 240;
    constexpr int frames = 1000;
    for (const auto scenario : {"static", "cursor", "full"}) {
        std::vector<std::byte> old_memory(fixed.smem_len), new_memory(fixed.smem_len);
        FramebufferBlitter blitter{fixed, info, new_memory};
        Canvas old_canvas{320, 170}, new_canvas{320, 170};
        const auto change = [&](Canvas& canvas, int frame) {
            if (std::string_view{scenario} == "cursor") canvas.setPixel(100, 80, Color{static_cast<std::uint8_t>(frame), 42, 42});
            if (std::string_view{scenario} == "full") canvas.clear(Color{static_cast<std::uint8_t>(frame), 42, 42});
        };
        (void)blitter.present(new_canvas);
        const auto before_old = clock::now();
        for (int frame = 0; frame < frames; ++frame) { change(old_canvas, frame); referencePresent(old_canvas, info, fixed, old_memory); }
        const auto before_new = clock::now();
        std::size_t written = 0;
        for (int frame = 0; frame < frames; ++frame) { change(new_canvas, frame); written += blitter.present(new_canvas); }
        const auto after = clock::now();
        expect(old_memory == new_memory, "benchmark output mismatch");
        std::cout << scenario << ": old_us/frame=" << std::chrono::duration<double, std::micro>(before_new - before_old).count() / frames
            << " new_us/frame=" << std::chrono::duration<double, std::micro>(after - before_new).count() / frames
            << " old_bytes/frame=" << fixed.smem_len + 320 * 170 * 2
            << " new_bytes/frame=" << written / frames << '\n';
    }
}
} // namespace

int main(int argc, char** argv)
{
    try {
        for (int bits : {16, 24, 32}) {
            for (int scale : {1, 2, 3}) {
                for (bool swap : {false, true}) verifyFormat(bits, scale, swap);
            }
        }
        verifyRejection();
        if (argc > 1 && std::string_view{argv[1]} == "--benchmark") benchmark();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
