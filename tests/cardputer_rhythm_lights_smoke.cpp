// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/device/cardputer_rhythm_lights.h"
#include "core/spectrum_display.h"

#include <cassert>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <limits>
#include <mutex>
#include <thread>
#include <vector>

int main()
{
    using namespace lofibox;
    using namespace std::chrono_literals;
    using State = platform::device::RhythmLightState;
    const auto frameForHeight = [](int height) {
        app::AudioVisualizationFrame frame{true, {}};
        const float fraction = static_cast<float>(height) / core::kSpectrumMaxHeight;
        frame.bands.back() = fraction * fraction * fraction * fraction / 4.0F;
        return frame;
    };
    // Known display heights, independent of the light state machine.
    assert(core::spectrumBarHeight(0.0F) == 0);
    assert(core::spectrumBarHeight(0.01F) == 51);
    assert(core::spectrumBarHeight(0.1F) == 91);
    assert(core::spectrumBarHeight(1.0F) == 114);

    platform::device::RhythmLightPattern pattern;
    for (const auto [height, lit] : std::vector<std::pair<int, bool>>{
        {50, false}, {53, false}, {54, true}, {56, true}, {53, true},
        {52, false}, {54, false}, {56, true}, {80, true}, {73, false},
        {79, true}, {76, true}, {75, false}, {0, false}}) {
        assert((pattern.advance(frameForHeight(height), true, .05) == State{lit, lit, lit}));
    }
    // Holding a crest holds the lamps. Time never adds flashes or cuts it off.
    assert((pattern.advance(frameForHeight(50), true, .05) == State{true, true, true}));
    for (int tick = 0; tick < 500; ++tick) {
        auto frame = frameForHeight(50);
        for (unsigned band = 0; band < 9; ++band) frame.bands[band] = tick % 2 ? 1.0F : 0.0F;
        assert((pattern.advance(frame, true, .05) == State{true, true, true}));
    }
    assert((pattern.advance(frameForHeight(46), true, .05) == State{}));
    for (int tick = 0; tick < 500; ++tick) {
        assert((pattern.advance(frameForHeight(46 + tick % 4), true, .05) == State{}));
    }
    assert((pattern.advance(frameForHeight(50), true, .05) == State{true, true, true}));
    assert((pattern.advance(frameForHeight(50), false, .05) == State{}));
    assert((pattern.advance(frameForHeight(50), true, .05) == State{}));
    assert((pattern.advance(frameForHeight(54), true, .05) == State{true, true, true}));
    assert((pattern.advance({}, true, .05) == State{}));
    assert((pattern.advance(frameForHeight(54), true, .05) == State{}));
    assert((pattern.advance(frameForHeight(58), true, .05) == State{true, true, true}));
    assert((pattern.advance(frameForHeight(80), true, 30.0) == State{}));
    assert((pattern.advance(frameForHeight(80), true, .05) == State{}));
    assert((pattern.advance(frameForHeight(84), true, .05) == State{true, true, true}));
    for (const float invalid : {std::numeric_limits<float>::quiet_NaN(),
                                std::numeric_limits<float>::infinity(), -1.0F}) {
        auto frame = frameForHeight(84);
        frame.bands.back() = invalid;
        assert((pattern.advance(frame, true, .05) == State{}));
    }
    assert((pattern.advance(frameForHeight(80), true, std::numeric_limits<double>::quiet_NaN()) == State{}));

    std::mutex mutex;
    std::condition_variable changed;
    bool entered = false;
    bool release = false;
    std::vector<State> outputs;
    const auto started = std::chrono::steady_clock::now();
    {
        platform::device::CardputerRhythmLights lights{"unused", [&](const auto& state) {
            std::unique_lock lock{mutex};
            entered = true;
            changed.notify_all();
            changed.wait(lock, [&] { return release; });
            outputs.push_back(state);
            changed.notify_all();
            return true;
        }};
        const auto frame = frameForHeight(60);
        lights.submit(frame, true);
        {
            std::unique_lock lock{mutex};
            assert(changed.wait_for(lock, 1s, [&] { return entered; }));
        }
        const auto publishing = std::chrono::steady_clock::now();
        for (int i = 0; i < 5000; ++i) lights.submit(frame, true);
        assert(std::chrono::steady_clock::now() - publishing < 1s);
        lights.submit({}, false);
        {
            const std::lock_guard lock{mutex};
            release = true;
        }
        changed.notify_all();
        {
            std::unique_lock lock{mutex};
            assert(changed.wait_for(lock, 1s, [&] { return outputs.size() >= 2 && outputs.back() == State{}; }));
        }
    }
    assert(std::chrono::steady_clock::now() - started < 3s);
    assert((outputs.back() == State{}));
    assert(outputs.size() < 10);
    {
        std::mutex mode_mutex;
        std::condition_variable mode_changed;
        std::vector<State> states;
        platform::device::CardputerRhythmLights lights{"unused", [&](const State& state) {
            const std::lock_guard lock{mode_mutex}; states.push_back(state); mode_changed.notify_all(); return true;
        }};
        lights.submit({}, true, platform::device::RhythmLightMode::Steady);
        {
            std::unique_lock lock{mode_mutex};
            assert(mode_changed.wait_for(lock, 1s, [&] { return !states.empty(); }));
            assert((states.back() == State{true, true, true}));
        }
        std::this_thread::sleep_for(150ms);
        { const std::lock_guard lock{mode_mutex}; assert(states.size() == 1); } // no steady-mode polling writes
        lights.submit({}, true, platform::device::RhythmLightMode::Off);
        {
            std::unique_lock lock{mode_mutex};
            assert(mode_changed.wait_for(lock, 1s, [&] { return states.size() > 1 && states.back() == State{}; }));
        }
        lights.submit(frameForHeight(50), true);
        {
            std::unique_lock lock{mode_mutex};
            assert(mode_changed.wait_for(lock, 1s, [&] { return states.size() > 2; }));
        }
        lights.submit(frameForHeight(60), true);
        {
            std::unique_lock lock{mode_mutex};
            assert(mode_changed.wait_for(lock, 1s, [&] { return states.back() == State{true, true, true}; }));
        }
    }
}
