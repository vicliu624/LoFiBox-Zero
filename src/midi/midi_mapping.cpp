// SPDX-License-Identifier: GPL-3.0-or-later

#include "midi/midi_mapping.h"

#include <cstddef>

namespace lofibox::midi {

GrooveMidiMapping defaultGrooveMidiMapping() noexcept
{
    GrooveMidiMapping mapping{};
    for (std::size_t index = 0; index < mapping.slotNotes.size(); ++index) {
        mapping.slotNotes[index] = static_cast<std::uint8_t>(36U + index);
    }
    mapping.ccTargets.emplace(static_cast<std::uint8_t>(20), MidiControlTarget::SelectedSlotGain);
    mapping.ccTargets.emplace(static_cast<std::uint8_t>(21), MidiControlTarget::SelectedSlotPitch);
    mapping.ccTargets.emplace(static_cast<std::uint8_t>(22), MidiControlTarget::FilterCutoff);
    mapping.ccTargets.emplace(static_cast<std::uint8_t>(23), MidiControlTarget::FxAmount);
    return mapping;
}

int slotForNote(const GrooveMidiMapping& mapping, std::uint8_t note) noexcept
{
    for (std::size_t index = 0; index < mapping.slotNotes.size(); ++index) {
        if (mapping.slotNotes[index] == note) {
            return static_cast<int>(index);
        }
    }
    return -1;
}

} // namespace lofibox::midi
