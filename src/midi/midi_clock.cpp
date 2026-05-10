// SPDX-License-Identifier: GPL-3.0-or-later

#include "midi/midi_clock.h"

namespace lofibox::midi {

void MidiClock::start() noexcept
{
    running_ = true;
    pulses_ = 0;
}

void MidiClock::stop() noexcept
{
    running_ = false;
}

void MidiClock::cont() noexcept
{
    running_ = true;
}

void MidiClock::tick() noexcept
{
    if (running_) {
        ++pulses_;
    }
}

bool MidiClock::running() const noexcept
{
    return running_;
}

std::uint64_t MidiClock::pulseCount() const noexcept
{
    return pulses_;
}

std::uint64_t MidiClock::stepCount() const noexcept
{
    return pulses_ / 6U;
}

} // namespace lofibox::midi
