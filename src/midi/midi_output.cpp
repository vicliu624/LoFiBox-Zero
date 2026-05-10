// SPDX-License-Identifier: GPL-3.0-or-later

#include "midi/midi_output.h"

namespace lofibox::midi {

void MidiOutputQueue::push(MidiMessage message)
{
    messages_.push_back(message);
}

const std::vector<MidiMessage>& MidiOutputQueue::messages() const noexcept
{
    return messages_;
}

void MidiOutputQueue::clear() noexcept
{
    messages_.clear();
}

} // namespace lofibox::midi
