// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/frame_deadline.h"
#include <iostream>

int main()
{
    using namespace std::chrono;
    using lofibox::core::nextFrameDeadline;
    const steady_clock::time_point start{};
    const auto interval = milliseconds{33};
    for (int elapsed : {0, 10, 32, 33, 34, 65, 66, 100, 249, 250, 1000, 60000}) {
        const auto completed = start + milliseconds{elapsed};
        const auto next = nextFrameDeadline(start, completed, interval);
        if (next < completed || next > completed + interval
            || (next - start) % interval != steady_clock::duration::zero()) {
            std::cerr << "Invalid frame deadline after " << elapsed << " ms\n";
            return 1;
        }
    }
    // Simulate a single 180 ms stall followed by inexpensive frames. Every
    // subsequent iteration must wait rather than rendering a catch-up burst.
    auto previous = start;
    for (int frame = 0; frame < 20; ++frame) {
        const auto completed = previous + milliseconds{frame == 0 ? 180 : 2};
        previous = nextFrameDeadline(previous, completed, interval);
        if (previous <= completed) return 1;
    }
    return 0;
}
