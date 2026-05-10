// SPDX-License-Identifier: GPL-3.0-or-later

#include "midi/midi_input_router.h"

#include <algorithm>

namespace lofibox::midi {

std::vector<lofibox::groove::PocketGrooveCommand> MidiInputRouter::route(
    const MidiMessage& message,
    const lofibox::groove::GrooveMidiSettings& settings,
    const GrooveMidiMapping& mapping) const
{
    using lofibox::groove::PocketGrooveCommand;
    using lofibox::groove::PocketGrooveCommandType;

    std::vector<PocketGrooveCommand> commands{};
    if (message.type == MidiMessageType::Clock) {
        return commands;
    }
    if (message.type == MidiMessageType::Start || message.type == MidiMessageType::Continue) {
        commands.push_back(lofibox::groove::makeGrooveCommand(PocketGrooveCommandType::PlayPause));
        return commands;
    }
    if (message.type == MidiMessageType::Stop) {
        commands.push_back(lofibox::groove::makeGrooveCommand(PocketGrooveCommandType::Stop));
        return commands;
    }

    if (message.channel != settings.inputChannel) {
        return commands;
    }
    if (message.type == MidiMessageType::NoteOn && settings.noteInputEnabled && message.data2 > 0U) {
        const int slot = slotForNote(mapping, message.data1);
        if (slot >= 0) {
            PocketGrooveCommand select = lofibox::groove::makeGrooveCommand(PocketGrooveCommandType::SelectSoundSlot);
            select.soundSlot = static_cast<std::uint8_t>(slot);
            PocketGrooveCommand trigger = lofibox::groove::makeGrooveCommand(PocketGrooveCommandType::TriggerSoundSlot);
            trigger.soundSlot = static_cast<std::uint8_t>(slot);
            trigger.intValue = std::clamp<int>(message.data2, 0, 127);
            commands.push_back(select);
            commands.push_back(trigger);
        }
        return commands;
    }
    if (message.type == MidiMessageType::ControlChange) {
        const auto found = mapping.ccTargets.find(message.data1);
        if (found == mapping.ccTargets.end()) {
            return commands;
        }
        PocketGrooveCommand command{};
        switch (found->second) {
        case MidiControlTarget::SelectedSlotGain:
            command = lofibox::groove::makeGrooveCommand(PocketGrooveCommandType::SetStepGain);
            command.floatValue = static_cast<float>(message.data2) / 64.0f;
            commands.push_back(command);
            break;
        case MidiControlTarget::SelectedSlotPitch:
            command = lofibox::groove::makeGrooveCommand(PocketGrooveCommandType::SetStepPitch);
            command.intValue = static_cast<int>(message.data2) - 64;
            commands.push_back(command);
            break;
        case MidiControlTarget::FilterCutoff:
        case MidiControlTarget::FxAmount:
        case MidiControlTarget::None:
            break;
        }
    }
    return commands;
}

} // namespace lofibox::midi
