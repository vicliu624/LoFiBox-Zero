// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/canvas.h"
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
using lofibox::core::Canvas;
using lofibox::core::Color;
using lofibox::core::rgba;

Canvas pattern(int width, int height)
{
    Canvas image{width, height};
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            image.setPixel(x, y, rgba(x * 13, y * 17, x * 7 + y * 11, (x + y) * 9));
    return image;
}

void equal(const Canvas& actual, const std::vector<Color>& expected)
{
    if (actual.pixels() != expected) throw std::runtime_error("clipped canvas pixels differ");
}

void copies()
{
    const auto original = pattern(17, 13);
    const auto source = pattern(11, 9);
    constexpr std::array extremes{std::numeric_limits<int>::min(), std::numeric_limits<int>::max()};
    std::vector<int> offsets(extremes.begin(), extremes.end());
    for (int n = -20; n <= 20; ++n) offsets.push_back(n);
    for (const bool self : {false, true}) {
        for (const int y : offsets) {
            for (const int x : offsets) {
                auto actual = original;
                auto expected = original.pixels();
                const auto& input = self ? expected : source.pixels();
                const int sw = self ? original.width() : source.width();
                const int sh = self ? original.height() : source.height();
                // Independent per-source-pixel reference, including the original
                // forward-copy semantics when the source aliases the destination.
                for (int sy = 0; sy < sh; ++sy) {
                    for (int sx = 0; sx < sw; ++sx) {
                        const auto dx = std::int64_t{x} + sx;
                        const auto dy = std::int64_t{y} + sy;
                        if (dx >= 0 && dx < original.width() && dy >= 0 && dy < original.height())
                            expected[dy * original.width() + dx] = input[sy * sw + sx];
                    }
                }
                actual.blit(self ? actual : source, x, y);
                equal(actual, expected);
            }
        }
    }
}

void rectangles()
{
    const auto original = pattern(17, 13);
    constexpr std::array values{std::numeric_limits<int>::min(), -30, -1, 0, 1, 7, 25,
                                std::numeric_limits<int>::max()};
    const auto color = rgba(123, 45, 67, 89);
    for (const int x : values) for (const int y : values)
        for (const int width : values) for (const int height : values) {
            auto actual = original;
            auto expected = original.pixels();
            for (int py = 0; py < original.height(); ++py)
                for (int px = 0; px < original.width(); ++px)
                    if (width > 0 && height > 0 && px >= x && py >= y
                        && px < std::int64_t{x} + width && py < std::int64_t{y} + height)
                        expected[py * original.width() + px] = color;
            actual.fillRect(x, y, width, height, color);
            equal(actual, expected);
        }
}
} // namespace

int main()
{
    try {
        copies();
        rectangles();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
