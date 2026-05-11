// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/linux_raw_midi_device_adapter.h"

#include <cassert>
#include <string>

int main()
{
    lofibox::platform::host::LinuxRawMidiDeviceAdapter adapter{};
    std::string error{};
    const bool opened = adapter.open(&error);
    const auto status = adapter.status();
    assert(status.available == opened);
    if (!opened) {
        assert(!error.empty() || !status.message.empty());
    }

    const auto messages = adapter.poll();
    (void)messages;
    const auto sent = adapter.send(lofibox::midi::MidiMessage{lofibox::midi::MidiMessageType::Clock});
    if (!opened) {
        assert(!sent);
    }

    adapter.close();
    assert(!adapter.available());
    return 0;
}
