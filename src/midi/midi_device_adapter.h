// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "midi/midi_input_router.h"

namespace lofibox::midi {

struct MidiDeviceStatus {
    bool available{false};
    std::filesystem::path inputPath{};
    std::filesystem::path outputPath{};
    std::string message{};
};

class MidiDeviceAdapter {
public:
    MidiDeviceAdapter() = default;
    MidiDeviceAdapter(std::filesystem::path input_path, std::filesystem::path output_path);
    ~MidiDeviceAdapter();

    MidiDeviceAdapter(const MidiDeviceAdapter&) = delete;
    MidiDeviceAdapter& operator=(const MidiDeviceAdapter&) = delete;
    MidiDeviceAdapter(MidiDeviceAdapter&&) = delete;
    MidiDeviceAdapter& operator=(MidiDeviceAdapter&&) = delete;

    [[nodiscard]] bool open(std::string* error = nullptr);
    void close() noexcept;
    [[nodiscard]] bool available() const noexcept;
    [[nodiscard]] MidiDeviceStatus status() const;

    [[nodiscard]] std::vector<MidiMessage> poll(std::size_t max_messages = 64);
    [[nodiscard]] bool send(const MidiMessage& message);

private:
    [[nodiscard]] std::vector<MidiMessage> parseByte(std::uint8_t byte);

    std::filesystem::path inputPath_{};
    std::filesystem::path outputPath_{};
    int inputFd_{-1};
    int outputFd_{-1};
    std::uint8_t runningStatus_{0};
    std::array<std::uint8_t, 2> pendingData_{};
    std::uint8_t pendingCount_{0};
    std::string lastMessage_{};
};

} // namespace lofibox::midi
