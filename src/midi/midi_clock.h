// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>

namespace lofibox::midi {

class MidiClock {
public:
    void start() noexcept;
    void stop() noexcept;
    void cont() noexcept;
    void tick() noexcept;

    [[nodiscard]] bool running() const noexcept;
    [[nodiscard]] std::uint64_t pulseCount() const noexcept;
    [[nodiscard]] std::uint64_t stepCount() const noexcept;

private:
    bool running_{false};
    std::uint64_t pulses_{0};
};

} // namespace lofibox::midi
