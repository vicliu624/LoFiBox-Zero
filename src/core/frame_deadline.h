// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <chrono>

namespace lofibox::core {

// Skip missed presentation slots: slow I/O must not trigger a burst of catch-up frames.
inline std::chrono::steady_clock::time_point nextFrameDeadline(
    std::chrono::steady_clock::time_point previous,
    std::chrono::steady_clock::time_point completed,
    std::chrono::steady_clock::duration interval)
{
    auto next = previous + interval;
    if (next < completed) {
        next += ((completed - next) / interval + 1) * interval;
    }
    return next;
}

} // namespace lofibox::core
