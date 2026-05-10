// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <vector>

#include "midi/midi_input_router.h"

namespace lofibox::midi {

class MidiOutputQueue {
public:
    void push(MidiMessage message);
    [[nodiscard]] const std::vector<MidiMessage>& messages() const noexcept;
    void clear() noexcept;

private:
    std::vector<MidiMessage> messages_{};
};

} // namespace lofibox::midi
