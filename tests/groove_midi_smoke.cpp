// SPDX-License-Identifier: GPL-3.0-or-later

#include "midi/midi_clock.h"
#include "midi/midi_device_adapter.h"
#include "midi/midi_input_router.h"
#include "midi/midi_mapping.h"

#include <cassert>

int main()
{
    const auto mapping = lofibox::midi::defaultGrooveMidiMapping();
    assert(lofibox::midi::slotForNote(mapping, 36) == 0);
    assert(lofibox::midi::slotForNote(mapping, 51) == 15);
    assert(lofibox::midi::slotForNote(mapping, 12) == -1);

    lofibox::groove::GrooveMidiSettings settings{};
    lofibox::groove::initializeDefaultSlotNotes(settings);
    lofibox::midi::MidiInputRouter router{};
    const auto commands = router.route({lofibox::midi::MidiMessageType::NoteOn, 10, 38, 100}, settings, mapping);
    assert(commands.size() == 2);
    assert(commands[0].type == lofibox::groove::PocketGrooveCommandType::SelectSoundSlot);
    assert(commands[0].soundSlot == 2);
    assert(commands[1].type == lofibox::groove::PocketGrooveCommandType::TriggerSoundSlot);

    const auto ignored = router.route({lofibox::midi::MidiMessageType::NoteOn, 1, 38, 100}, settings, mapping);
    assert(ignored.empty());

    lofibox::midi::MidiClock clock{};
    clock.start();
    for (int i = 0; i < 12; ++i) {
        clock.tick();
    }
    assert(clock.running());
    assert(clock.pulseCount() == 12);
    assert(clock.stepCount() == 2);
    clock.stop();
    assert(!clock.running());

    lofibox::midi::MidiDeviceAdapter adapter{};
    std::string error{};
    const bool opened = adapter.open(&error);
    const auto status = adapter.status();
    assert(status.available == opened);
    if (!opened) {
        assert(!error.empty());
    }
    adapter.close();
    assert(!adapter.available());

    return 0;
}
