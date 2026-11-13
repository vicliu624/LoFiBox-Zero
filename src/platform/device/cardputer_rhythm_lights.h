// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <stop_token>
#include <string>
#include <thread>

#include "app/audio_visualization.h"

namespace lofibox::platform::device {

using RhythmLightState = std::array<bool, 3>;
enum class RhythmLightMode { Off, Steady, Music };

class RhythmLightPattern {
public:
    [[nodiscard]] RhythmLightState advance(const app::AudioVisualizationFrame& frame, bool playing, double seconds) noexcept;
private:
    int extreme_height_{};
    bool initialized_{};
    bool lit_{};
};

// The callback runs exclusively on the light worker, including the final off
// update. GUI publication never performs I2C, input ioctls or process creation.
class CardputerRhythmLights {
public:
    using Output = std::function<bool(const RhythmLightState&)>;
    explicit CardputerRhythmLights(std::string keyboard_device, Output output = {});
    ~CardputerRhythmLights();
    void submit(const app::AudioVisualizationFrame& frame, bool playing,
                RhythmLightMode mode = RhythmLightMode::Music);
private:
    void run(std::stop_token stop);
    Output output_;
    std::mutex mutex_;
    std::condition_variable_any changed_;
    app::AudioVisualizationFrame latest_{};
    bool playing_{};
    RhythmLightMode mode_{RhythmLightMode::Music};
    std::jthread worker_;
};

} // namespace lofibox::platform::device
