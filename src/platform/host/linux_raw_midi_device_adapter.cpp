// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/linux_raw_midi_device_adapter.h"

#include <algorithm>
#include <utility>

#if defined(__linux__)
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace lofibox::platform::host {
namespace {

#if defined(__linux__)
[[nodiscard]] std::filesystem::path defaultRawMidiPath()
{
    const std::filesystem::path base{"/dev/snd"};
    std::error_code ec{};
    if (!std::filesystem::exists(base, ec)) {
        return {};
    }
    for (const auto& entry : std::filesystem::directory_iterator(base, ec)) {
        if (ec) {
            break;
        }
        const auto name = entry.path().filename().string();
        if (name.rfind("midiC", 0) == 0) {
            return entry.path();
        }
    }
    return {};
}

[[nodiscard]] bool writeAll(int fd, const std::array<std::uint8_t, 3>& bytes, std::size_t count)
{
    std::size_t written = 0;
    while (written < count) {
        const auto result = ::write(fd, bytes.data() + written, count - written);
        if (result < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            return false;
        }
        written += static_cast<std::size_t>(result);
    }
    return true;
}
#endif

[[nodiscard]] std::uint8_t channelStatus(std::uint8_t high_nibble, std::uint8_t channel)
{
    const auto safe_channel = static_cast<std::uint8_t>(std::clamp<int>(channel, 1, 16) - 1);
    return static_cast<std::uint8_t>(high_nibble | safe_channel);
}

[[nodiscard]] std::size_t expectedDataBytes(std::uint8_t status)
{
    const auto high = status & 0xF0U;
    if (high == 0xC0U || high == 0xD0U) {
        return 1;
    }
    if (high >= 0x80U && high <= 0xE0U) {
        return 2;
    }
    return 0;
}

} // namespace

LinuxRawMidiDeviceAdapter::LinuxRawMidiDeviceAdapter(std::filesystem::path input_path, std::filesystem::path output_path)
    : inputPath_(std::move(input_path))
    , outputPath_(std::move(output_path))
{}

LinuxRawMidiDeviceAdapter::~LinuxRawMidiDeviceAdapter()
{
    close();
}

bool LinuxRawMidiDeviceAdapter::open(std::string* error)
{
    close();
    lastMessage_.clear();
#if defined(__linux__)
    if (inputPath_.empty()) {
        inputPath_ = defaultRawMidiPath();
    }
    if (outputPath_.empty()) {
        outputPath_ = inputPath_;
    }
    if (inputPath_.empty() && outputPath_.empty()) {
        lastMessage_ = "no Linux raw MIDI device found under /dev/snd";
        if (error != nullptr) *error = lastMessage_;
        return false;
    }
    if (!inputPath_.empty()) {
        inputFd_ = ::open(inputPath_.c_str(), O_RDONLY | O_NONBLOCK);
        if (inputFd_ < 0) {
            lastMessage_ = std::string{"could not open MIDI input: "} + std::strerror(errno);
            if (error != nullptr) *error = lastMessage_;
        }
    }
    if (!outputPath_.empty()) {
        outputFd_ = ::open(outputPath_.c_str(), O_WRONLY | O_NONBLOCK);
        if (outputFd_ < 0 && inputFd_ >= 0) {
            lastMessage_ = std::string{"MIDI output unavailable: "} + std::strerror(errno);
        } else if (outputFd_ < 0 && inputFd_ < 0) {
            lastMessage_ = std::string{"could not open MIDI output: "} + std::strerror(errno);
            if (error != nullptr) *error = lastMessage_;
            return false;
        }
    }
    if (available() && lastMessage_.empty()) {
        lastMessage_ = "raw MIDI device open";
    }
    return available();
#else
    lastMessage_ = "Linux raw MIDI adapter is unavailable on this platform";
    if (error != nullptr) *error = lastMessage_;
    return false;
#endif
}

void LinuxRawMidiDeviceAdapter::close() noexcept
{
#if defined(__linux__)
    if (inputFd_ >= 0) {
        ::close(inputFd_);
    }
    if (outputFd_ >= 0 && outputFd_ != inputFd_) {
        ::close(outputFd_);
    }
#endif
    inputFd_ = -1;
    outputFd_ = -1;
    runningStatus_ = 0;
    pendingCount_ = 0;
}

bool LinuxRawMidiDeviceAdapter::available() const noexcept
{
    return inputFd_ >= 0 || outputFd_ >= 0;
}

lofibox::midi::MidiPortStatus LinuxRawMidiDeviceAdapter::status() const
{
    return lofibox::midi::MidiPortStatus{available(), inputPath_.string(), outputPath_.string(), lastMessage_};
}

std::vector<lofibox::midi::MidiMessage> LinuxRawMidiDeviceAdapter::poll(std::size_t max_messages)
{
    std::vector<lofibox::midi::MidiMessage> messages{};
#if defined(__linux__)
    if (inputFd_ < 0 || max_messages == 0U) {
        return messages;
    }
    std::array<std::uint8_t, 128> buffer{};
    while (messages.size() < max_messages) {
        const auto read_bytes = ::read(inputFd_, buffer.data(), buffer.size());
        if (read_bytes < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                lastMessage_ = std::string{"MIDI read failed: "} + std::strerror(errno);
            }
            break;
        }
        if (read_bytes == 0) {
            break;
        }
        for (std::size_t index = 0; index < static_cast<std::size_t>(read_bytes) && messages.size() < max_messages; ++index) {
            auto parsed = parseByte(buffer[index]);
            messages.insert(messages.end(), parsed.begin(), parsed.end());
        }
    }
#else
    (void)max_messages;
#endif
    return messages;
}

bool LinuxRawMidiDeviceAdapter::send(const lofibox::midi::MidiMessage& message)
{
#if defined(__linux__)
    if (outputFd_ < 0) {
        lastMessage_ = "MIDI output unavailable";
        return false;
    }
    std::array<std::uint8_t, 3> bytes{};
    std::size_t count = 0;
    switch (message.type) {
    case lofibox::midi::MidiMessageType::Clock:
        bytes[0] = 0xF8U;
        count = 1;
        break;
    case lofibox::midi::MidiMessageType::Start:
        bytes[0] = 0xFAU;
        count = 1;
        break;
    case lofibox::midi::MidiMessageType::Continue:
        bytes[0] = 0xFBU;
        count = 1;
        break;
    case lofibox::midi::MidiMessageType::Stop:
        bytes[0] = 0xFCU;
        count = 1;
        break;
    case lofibox::midi::MidiMessageType::NoteOn:
        bytes = {channelStatus(0x90U, message.channel), message.data1, message.data2};
        count = 3;
        break;
    case lofibox::midi::MidiMessageType::NoteOff:
        bytes = {channelStatus(0x80U, message.channel), message.data1, message.data2};
        count = 3;
        break;
    case lofibox::midi::MidiMessageType::ControlChange:
        bytes = {channelStatus(0xB0U, message.channel), message.data1, message.data2};
        count = 3;
        break;
    }
    const bool ok = writeAll(outputFd_, bytes, count);
    if (!ok) {
        lastMessage_ = std::string{"MIDI write failed: "} + std::strerror(errno);
    }
    return ok;
#else
    (void)message;
    return false;
#endif
}

std::vector<lofibox::midi::MidiMessage> LinuxRawMidiDeviceAdapter::parseByte(std::uint8_t byte)
{
    std::vector<lofibox::midi::MidiMessage> messages{};
    switch (byte) {
    case 0xF8U:
        messages.push_back(lofibox::midi::MidiMessage{lofibox::midi::MidiMessageType::Clock});
        return messages;
    case 0xFAU:
        messages.push_back(lofibox::midi::MidiMessage{lofibox::midi::MidiMessageType::Start});
        return messages;
    case 0xFBU:
        messages.push_back(lofibox::midi::MidiMessage{lofibox::midi::MidiMessageType::Continue});
        return messages;
    case 0xFCU:
        messages.push_back(lofibox::midi::MidiMessage{lofibox::midi::MidiMessageType::Stop});
        return messages;
    default:
        break;
    }
    if ((byte & 0x80U) != 0U) {
        runningStatus_ = byte;
        pendingCount_ = 0;
        return messages;
    }
    if (runningStatus_ == 0U) {
        return messages;
    }
    const auto expected = expectedDataBytes(runningStatus_);
    if (expected == 0U) {
        return messages;
    }
    pendingData_[pendingCount_++] = byte;
    if (pendingCount_ < expected) {
        return messages;
    }

    const auto type_nibble = runningStatus_ & 0xF0U;
    const auto channel = static_cast<std::uint8_t>((runningStatus_ & 0x0FU) + 1U);
    if (type_nibble == 0x90U) {
        messages.push_back(lofibox::midi::MidiMessage{pendingData_[1] == 0U ? lofibox::midi::MidiMessageType::NoteOff : lofibox::midi::MidiMessageType::NoteOn, channel, pendingData_[0], pendingData_[1]});
    } else if (type_nibble == 0x80U) {
        messages.push_back(lofibox::midi::MidiMessage{lofibox::midi::MidiMessageType::NoteOff, channel, pendingData_[0], pendingData_[1]});
    } else if (type_nibble == 0xB0U) {
        messages.push_back(lofibox::midi::MidiMessage{lofibox::midi::MidiMessageType::ControlChange, channel, pendingData_[0], pendingData_[1]});
    }
    pendingCount_ = 0;
    return messages;
}

} // namespace lofibox::platform::host
