// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "midi/midi_message.h"

namespace lofibox::midi {

struct MidiPortStatus {
    bool available{false};
    std::string inputPath{};
    std::string outputPath{};
    std::string message{};
};

class MidiPort {
public:
    virtual ~MidiPort() = default;

    [[nodiscard]] virtual bool open(std::string* error = nullptr) = 0;
    virtual void close() noexcept = 0;
    [[nodiscard]] virtual bool available() const noexcept = 0;
    [[nodiscard]] virtual MidiPortStatus status() const = 0;

    [[nodiscard]] virtual std::vector<MidiMessage> poll(std::size_t max_messages = 64) = 0;
    [[nodiscard]] virtual bool send(const MidiMessage& message) = 0;
};

} // namespace lofibox::midi
