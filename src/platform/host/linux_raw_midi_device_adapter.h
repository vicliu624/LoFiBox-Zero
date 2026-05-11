// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "midi/midi_input_router.h"

namespace lofibox::platform::host {

struct MidiDeviceStatus {
    bool available{false};
    std::filesystem::path inputPath{};
    std::filesystem::path outputPath{};
    std::string message{};
};

class LinuxRawMidiDeviceAdapter {
public:
    LinuxRawMidiDeviceAdapter() = default;
    LinuxRawMidiDeviceAdapter(std::filesystem::path input_path, std::filesystem::path output_path);
    ~LinuxRawMidiDeviceAdapter();

    LinuxRawMidiDeviceAdapter(const LinuxRawMidiDeviceAdapter&) = delete;
    LinuxRawMidiDeviceAdapter& operator=(const LinuxRawMidiDeviceAdapter&) = delete;
    LinuxRawMidiDeviceAdapter(LinuxRawMidiDeviceAdapter&&) = delete;
    LinuxRawMidiDeviceAdapter& operator=(LinuxRawMidiDeviceAdapter&&) = delete;

    [[nodiscard]] bool open(std::string* error = nullptr);
    void close() noexcept;
    [[nodiscard]] bool available() const noexcept;
    [[nodiscard]] MidiDeviceStatus status() const;

    [[nodiscard]] std::vector<lofibox::midi::MidiMessage> poll(std::size_t max_messages = 64);
    [[nodiscard]] bool send(const lofibox::midi::MidiMessage& message);

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
