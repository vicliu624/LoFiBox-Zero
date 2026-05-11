// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>

namespace lofibox::midi {

enum class MidiMessageType {
    NoteOn,
    NoteOff,
    ControlChange,
    Clock,
    Start,
    Stop,
    Continue
};

struct MidiMessage {
    MidiMessageType type{MidiMessageType::NoteOn};
    std::uint8_t channel{10};
    std::uint8_t data1{0};
    std::uint8_t data2{0};
};

} // namespace lofibox::midi
