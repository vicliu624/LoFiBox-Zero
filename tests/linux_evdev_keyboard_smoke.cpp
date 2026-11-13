// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include <chrono>
#include <filesystem>
#include <thread>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <linux/input.h>

#include "platform/device/linux_evdev_keyboard.h"

namespace {

bool expectKey(unsigned int keycode, lofibox::app::InputKey expected, const char* label)
{
    const auto translated = lofibox::platform::device::translateLinuxEvdevCommandKey(keycode);
    if (!translated || translated->key != expected) {
        std::cerr << "Expected " << label << " to translate to the matching logical command key.\n";
        return false;
    }
    return true;
}

bool expectNoCommand(unsigned int keycode, const char* label)
{
    const auto translated = lofibox::platform::device::translateLinuxEvdevCommandKey(keycode);
    if (translated) {
        std::cerr << "Expected " << label << " to remain a character key, not a direction command.\n";
        return false;
    }
    return true;
}

} // namespace

int main()
{
    if (!expectKey(KEY_ESC, lofibox::app::InputKey::Escape, "KEY_ESC")) return 1;
    if (!expectKey(KEY_HELP, lofibox::app::InputKey::F1, "KEY_HELP / Fn+H")) return 1;
    if (!expectKey(KEY_PLAYPAUSE, lofibox::app::InputKey::MediaPlayPause, "Fn+Q")) return 1;
    if (!expectKey(KEY_PREVIOUSSONG, lofibox::app::InputKey::MediaPrevious, "Fn+W")) return 1;
    if (!expectKey(KEY_NEXTSONG, lofibox::app::InputKey::MediaNext, "Fn+E")) return 1;
    if (!expectKey(KEY_VOLUMEDOWN, lofibox::app::InputKey::VolumeDown, "Fn+S")) return 1;
    if (!expectKey(KEY_VOLUMEUP, lofibox::app::InputKey::VolumeUp, "Fn+D")) return 1;
    if (!expectKey(KEY_MUTE, lofibox::app::InputKey::VolumeMute, "Fn+A")) return 1;
    if (!expectKey(KEY_BRIGHTNESSDOWN, lofibox::app::InputKey::BrightnessDown, "Fn+U")) return 1;
    if (!expectKey(KEY_BRIGHTNESSUP, lofibox::app::InputKey::BrightnessUp, "Fn+I")) return 1;
    if (!expectKey(KEY_UP, lofibox::app::InputKey::Up, "KEY_UP")) return 1;
    if (!expectKey(KEY_DOWN, lofibox::app::InputKey::Down, "KEY_DOWN")) return 1;
    if (!expectKey(KEY_LEFT, lofibox::app::InputKey::Left, "KEY_LEFT")) return 1;
    if (!expectKey(KEY_RIGHT, lofibox::app::InputKey::Right, "KEY_RIGHT")) return 1;

    if (!expectNoCommand(KEY_X, "KEY_X")) return 1;
    if (!expectNoCommand(KEY_F, "KEY_F")) return 1;
    if (!expectNoCommand(KEY_S, "KEY_S")) return 1;
    if (!expectNoCommand(KEY_Z, "KEY_Z")) return 1;
    if (!expectNoCommand(KEY_C, "KEY_C")) return 1;

    const auto fifo = std::filesystem::temp_directory_path() / ("lofibox-evdev-" + std::to_string(::getpid()));
    if (::mkfifo(fifo.c_str(), 0600) != 0) return 1;
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::filesystem::remove(path); }
    } cleanup{fifo};
    lofibox::platform::device::LinuxEvdevKeyboard keyboard{fifo.string()};
    const int writer = ::open(fifo.c_str(), O_WRONLY | O_NONBLOCK);
    if (writer < 0 || !keyboard.available()) return 1;
    const auto send = [writer](unsigned short code, int value) {
        input_event event{};
        event.type = EV_KEY; event.code = code; event.value = value;
        return ::write(writer, &event, sizeof(event)) == sizeof(event);
    };
    // The wake key and its entire held/repeat/release sequence are consumed.
    if (!send(KEY_NEXTSONG, 1) || !keyboard.drainInput(true).empty() || !keyboard.inputActivity()) return 1;
    if (!send(KEY_NEXTSONG, 2) || !keyboard.drainInput().empty()) return 1;
    if (!send(KEY_NEXTSONG, 0) || !keyboard.drainInput().empty()) return 1;
    if (!send(KEY_NEXTSONG, 1)) return 1;
    const auto after_wake = keyboard.drainInput();
    if (after_wake.size() != 1 || after_wake.front().key != lofibox::app::InputKey::MediaNext) return 1;
    if (!send(KEY_NEXTSONG, 0) || !keyboard.drainInput().empty()) return 1;
    if (!send(KEY_ESC, 1) || !keyboard.drainInput(true).empty()) return 1;
    if (keyboard.escHeldDuration() != std::chrono::milliseconds::zero()) return 1;
    if (!send(KEY_ESC, 0) || !keyboard.drainInput().empty()) return 1;
    if (keyboard.inputActivity()) return 1;
    if (!send(KEY_ESC, 1) || !keyboard.drainInput().empty()) return 1;
    if (!send(KEY_ESC, 2) || !keyboard.drainInput().empty()) return 1;
    if (!send(KEY_ESC, 0)) return 1;
    auto events = keyboard.drainInput();
    if (events.size() != 1 || events.front().key != lofibox::app::InputKey::Escape
        || keyboard.escHeldDuration() != std::chrono::milliseconds::zero()) return 1;
    if (!send(KEY_PLAYPAUSE, 2)) return 1;
    events = keyboard.drainInput();
    if (events.size() != 1 || !events.front().repeated || events.front().key != lofibox::app::InputKey::MediaPlayPause) return 1;
    if (!send(KEY_ESC, 1) || !keyboard.drainInput().empty()) return 1;
    std::this_thread::sleep_for(std::chrono::milliseconds{2050});
    if (keyboard.escHeldDuration() < std::chrono::seconds{2}) return 1;
    if (!send(KEY_ESC, 0) || !keyboard.drainInput().empty()) return 1;
    ::close(writer);
    return 0;
}
