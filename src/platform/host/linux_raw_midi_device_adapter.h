// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "midi/midi_port.h"

namespace lofibox::platform::host {

class LinuxRawMidiDeviceAdapter final : public lofibox::midi::MidiPort {
public:
    LinuxRawMidiDeviceAdapter() = default;
    LinuxRawMidiDeviceAdapter(std::filesystem::path input_path, std::filesystem::path output_path);
    ~LinuxRawMidiDeviceAdapter();

    LinuxRawMidiDeviceAdapter(const LinuxRawMidiDeviceAdapter&) = delete;
    LinuxRawMidiDeviceAdapter& operator=(const LinuxRawMidiDeviceAdapter&) = delete;
    LinuxRawMidiDeviceAdapter(LinuxRawMidiDeviceAdapter&&) = delete;
    LinuxRawMidiDeviceAdapter& operator=(LinuxRawMidiDeviceAdapter&&) = delete;

    [[nodiscard]] bool open(std::string* error = nullptr) override;
    void close() noexcept override;
    [[nodiscard]] bool available() const noexcept override;
    [[nodiscard]] lofibox::midi::MidiPortStatus status() const override;

    [[nodiscard]] std::vector<lofibox::midi::MidiMessage> poll(std::size_t max_messages = 64) override;
    [[nodiscard]] bool send(const lofibox::midi::MidiMessage& message) override;

private:
    [[nodiscard]] std::vector<lofibox::midi::MidiMessage> parseByte(std::uint8_t byte);

    std::filesystem::path inputPath_{};
    std::filesystem::path outputPath_{};
    int inputFd_{-1};
    int outputFd_{-1};
    std::uint8_t runningStatus_{0};
    std::array<std::uint8_t, 2> pendingData_{};
    std::uint8_t pendingCount_{0};
    std::string lastMessage_{};
};

} // namespace lofibox::platform::host
