// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <vector>

#include "groove/groove_commands.h"
#include "groove/groove_project.h"
#include "midi/midi_mapping.h"

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

class MidiInputRouter {
public:
    [[nodiscard]] std::vector<lofibox::groove::PocketGrooveCommand> route(
        const MidiMessage& message,
        const lofibox::groove::GrooveMidiSettings& settings,
        const GrooveMidiMapping& mapping) const;
};

} // namespace lofibox::midi
