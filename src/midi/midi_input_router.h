// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <vector>

#include "groove/groove_commands.h"
#include "groove/groove_project.h"
#include "midi/midi_message.h"
#include "midi/midi_mapping.h"

namespace lofibox::midi {

class MidiInputRouter {
public:
    [[nodiscard]] std::vector<lofibox::groove::PocketGrooveCommand> route(
        const MidiMessage& message,
        const lofibox::groove::GrooveMidiSettings& settings,
        const GrooveMidiMapping& mapping) const;
};

} // namespace lofibox::midi
