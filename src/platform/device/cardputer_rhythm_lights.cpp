// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/device/cardputer_rhythm_lights.h"
#include "core/spectrum_display.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace lofibox::platform::device {
namespace {

// Official CP0 keyboard firmware: BD=Shift, BE=Fn, BF=Sym; 0=off,
// 1=one-shot, 2=locked, 3=held/on. These are white single-color LEDs.
// No GPIO direction, PWM frequency, backlight, reset or power register is used.
class KeyboardLights {
public:
    explicit KeyboardLights(std::string keyboard) : keyboard_path_(std::move(keyboard)) {}
    ~KeyboardLights()
    {
        try { if (i2c_ >= 0 && keyboard_ >= 0) restore(); } catch (...) {}
        if (i2c_ >= 0) ::close(i2c_);
        if (keyboard_ >= 0) ::close(keyboard_);
    }

    bool apply(const RhythmLightState& state)
    {
        if (i2c_ < 0) open();
        const auto active = modifiers();
        const auto modes = readModes();
        const bool keyboard_active = std::any_of(active.begin(), active.end(), [](bool v) { return v; });
        for (std::size_t i = 0; i < 3; ++i) {
            if (active[i] || modes[i] == 1 || modes[i] == 2) {
                owned_[i] = false;
                continue;
            }
            if (keyboard_active && !owned_[i]) continue;
            const auto value = static_cast<std::uint8_t>(!keyboard_active && state[i] ? 3 : 0);
            if (modes[i] != value) {
                // Recheck immediately before each write, rather than retaining
                // a keyboard state sampled at the beginning of the batch.
                if (modifiers()[i]) { owned_[i] = false; continue; }
                writeMode(i, value);
                owned_[i] = true;
                last_[i] = value;
            }
        }
        // A newly pressed modifier can race the small I2C batch. Its initial
        // indication is steady ON; subsequent one-shot/lock transitions remain
        // entirely owned by the kernel keyboard state machine.
        const auto after = modifiers();
        const auto current = readModes();
        for (std::size_t i = 0; i < 3; ++i) {
            if (after[i] && owned_[i]) {
                if (current[i] == 0 && last_[i] == 0) writeMode(i, 3);
                owned_[i] = false;
            }
        }
        return true;
    }

private:
    static std::uint32_t be32(const std::vector<unsigned char>& value, std::size_t offset)
    {
        if (offset + 4 > value.size()) throw std::runtime_error("invalid CP0 device tree");
        return (std::uint32_t{value[offset]} << 24) | (std::uint32_t{value[offset+1]} << 16)
            | (std::uint32_t{value[offset+2]} << 8) | value[offset+3];
    }
    static std::vector<unsigned char> property(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary);
        if (!input) throw std::runtime_error("CP0 LED metadata unavailable");
        return {std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
    }
    void open()
    {
        if (keyboard_ >= 0) { ::close(keyboard_); keyboard_ = -1; }
        const std::filesystem::path base{"/sys/bus/i2c/devices/1-0034/of_node"};
        const auto phandle = be32(property("/sys/bus/i2c/devices/1-004f/of_node/phandle"), 0);
        const auto map = property(base / "linux,keymap");
        constexpr std::array names{"capslock-led-mode", "fn-led-mode", "tables-sel-led-mode"};
        constexpr std::array buttons{"shift-button-code", "fn-button-code", "sym-button-code"};
        for (std::size_t i = 0; i < 3; ++i) {
            const auto mode = property(base / names[i]);
            if (mode.size() != 8 || be32(mode, 0) != phandle || be32(mode, 4) != 0xbd + i)
                throw std::runtime_error("unsupported CP0 LED mapping");
            const auto scan = be32(property(base / buttons[i]), 0);
            bool found = false;
            for (std::size_t n = 0; n + 4 <= map.size(); n += 4) {
                const auto key = be32(map, n);
                if ((((key >> 24) & 0xff) << 4 | ((key >> 16) & 0xff)) == scan) {
                    keycodes_[i] = key & 0xffff;
                    found = keycodes_[i] > 0 && keycodes_[i] <= KEY_MAX;
                    break;
                }
            }
            if (!found) throw std::runtime_error("CP0 modifier key mapping unavailable");
        }
        keyboard_ = ::open(keyboard_path_.c_str(), O_RDONLY | O_CLOEXEC | O_NONBLOCK);
        if (keyboard_ < 0) throw std::runtime_error("cannot read CP0 modifier state");
        std::array<char, 128> name{};
        if (::ioctl(keyboard_, EVIOCGNAME(name.size()), name.data()) < 0 || std::string_view{name.data()} != "tca8418c")
            throw std::runtime_error("rhythm lights require the CP0 internal keyboard");
        i2c_ = ::open("/dev/i2c-1", O_RDWR | O_CLOEXEC);
        if (i2c_ < 0) throw std::runtime_error("cannot access CP0 light controller");
        // All writes below are independently limited to the three validated
        // LED-mode registers. No forced slave access or driver unbinding.
        (void)readModes();
    }
    RhythmLightState modifiers() const
    {
        std::array<unsigned char, (KEY_MAX + 8) / 8> bits{};
        if (::ioctl(keyboard_, EVIOCGKEY(bits.size()), bits.data()) < 0)
            throw std::runtime_error("CP0 modifier read failed");
        RhythmLightState result{};
        for (std::size_t i = 0; i < 3; ++i) result[i] = (bits[keycodes_[i]/8] & (1U << (keycodes_[i]%8))) != 0;
        return result;
    }
    std::array<std::uint8_t, 3> readModes() const
    {
        std::uint8_t address{0xbd};
        std::array<std::uint8_t, 3> values{};
        i2c_msg messages[]{{0x4f, 0, 1, &address}, {0x4f, I2C_M_RD, 3, values.data()}};
        i2c_rdwr_ioctl_data data{messages, 2};
        if (::ioctl(i2c_, I2C_RDWR, &data) != 2) throw std::runtime_error("CP0 light read failed");
        if (std::any_of(values.begin(), values.end(), [](auto mode) { return mode > 3; }))
            throw std::runtime_error("unsupported CP0 light protocol");
        return values;
    }
    void writeMode(std::size_t index, std::uint8_t mode) const
    {
        if (index >= 3 || (mode != 0 && mode != 3)) throw std::runtime_error("invalid CP0 light command");
        std::uint8_t bytes[]{static_cast<std::uint8_t>(0xbd + index), mode};
        i2c_msg message{0x4f, 0, 2, bytes};
        i2c_rdwr_ioctl_data data{&message, 1};
        if (::ioctl(i2c_, I2C_RDWR, &data) != 1) throw std::runtime_error("CP0 light write failed");
    }
    void restore()
    {
        const auto active = modifiers();
        const auto modes = readModes();
        for (std::size_t i = 0; i < 3; ++i) {
            if (owned_[i] && !active[i] && modes[i] == last_[i]) writeMode(i, 0);
        }
    }
    std::string keyboard_path_;
    int i2c_{-1};
    int keyboard_{-1};
    std::array<unsigned, 3> keycodes_{};
    RhythmLightState owned_{};
    std::array<std::uint8_t, 3> last_{};
};

} // namespace

RhythmLightState RhythmLightPattern::advance(const app::AudioVisualizationFrame& frame, bool playing, double seconds) noexcept
{
    if (!playing || !frame.available || !std::isfinite(seconds) || seconds < 0.0 || seconds > 0.25) {
        *this = RhythmLightPattern{};
        return {};
    }
    // Now Playing's last column interpolates to bands.back() exactly.
    const int height = core::spectrumBarHeight(frame.bands.back());
    if (!initialized_) {
        initialized_ = true;
        extreme_height_ = height;
        return {};
    }
    constexpr int hysteresis = 4;
    if (height < 10) {
        lit_ = false;
        extreme_height_ = height;
    } else if (lit_) {
        extreme_height_ = std::max(extreme_height_, height);
        if (height <= extreme_height_ - hysteresis) {
            lit_ = false;
            extreme_height_ = height;
        }
    } else {
        extreme_height_ = std::min(extreme_height_, height);
        if (height >= extreme_height_ + hysteresis) {
            lit_ = true;
            extreme_height_ = height;
        }
    }
    // No beat estimate or pulse timer: hold the visible column's rising/falling
    // state, with four pixels of reversal hysteresis to reject tiny jitter.
    return {lit_, lit_, lit_};
}

CardputerRhythmLights::CardputerRhythmLights(std::string keyboard, Output output) : output_(std::move(output))
{
    if (!output_) {
        auto hardware = std::make_shared<KeyboardLights>(std::move(keyboard));
        output_ = [hardware](const RhythmLightState& state) { return hardware->apply(state); };
    }
}

CardputerRhythmLights::~CardputerRhythmLights()
{
    worker_.request_stop();
    changed_.notify_all();
}

void CardputerRhythmLights::submit(const app::AudioVisualizationFrame& frame, bool playing)
{
    const std::lock_guard lock{mutex_};
    latest_ = frame;
    playing_ = playing && frame.available;
    if (!worker_.joinable() && playing_) worker_ = std::jthread([this](std::stop_token stop) { run(stop); });
    changed_.notify_all();
}

void CardputerRhythmLights::run(std::stop_token stop)
{
    using clock = std::chrono::steady_clock;
    RhythmLightPattern pattern;
    auto previous = clock::now();
    try {
        while (!stop.stop_requested()) {
            app::AudioVisualizationFrame frame;
            bool playing;
            {
                std::unique_lock lock{mutex_};
                if (!playing_ && !changed_.wait(lock, stop, [this] { return playing_; })) break;
                frame = latest_; playing = playing_;
            }
            const auto now = clock::now();
            const auto state = pattern.advance(frame, playing, std::chrono::duration<double>(now - previous).count());
            previous = now;
            if (!output_(state)) break;
            std::unique_lock lock{mutex_};
            changed_.wait_until(lock, stop, now + std::chrono::milliseconds{50}, [this, playing] { return playing_ != playing; });
            if (!playing_ && playing) {
                lock.unlock();
                (void)pattern.advance({}, false, 0.0);
                if (!output_({})) break;
            }
        }
        output_({});
    } catch (const std::exception& error) {
        std::cerr << "Rhythm lights stopped: " << error.what() << '\n';
        try { output_({}); } catch (...) {}
    }
}

} // namespace lofibox::platform::device
