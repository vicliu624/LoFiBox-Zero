// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/runtime_preferences.h"
#include "platform/host/runtime_preferences_store.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {
using lofibox::app::RuntimeFeature;
using lofibox::app::RuntimePreferences;
void require(bool condition, const char* reason)
{
    if (!condition) throw std::runtime_error(reason);
}
}

int main()
{
    using namespace std::chrono_literals;
    const auto directory = std::filesystem::temp_directory_path()
        / ("lofibox-preferences-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto path = directory / "runtime-preferences.conf";
    try {
        auto preferences = lofibox::platform::host::loadRuntimePreferences(path);
        preferences->configureWebUiDefault(true);
        require(preferences->effective(RuntimeFeature::WebUi), "new profile ignores target WebUI default");
        require(preferences->toggle(RuntimeFeature::WebUi), "cannot save interface preference");
        preferences->configureWebUiDefault(true);
        require(!preferences->effective(RuntimeFeature::WebUi), "launcher default overrides user's OFF");
        const auto normal = preferences->configured();
        require(preferences->toggle(RuntimeFeature::BatterySaver), "cannot enable battery mode");
        for (const auto feature : {RuntimeFeature::WebUi, RuntimeFeature::ExternalControl,
             RuntimeFeature::Spectrum, RuntimeFeature::Leds, RuntimeFeature::LedAnimation})
            require(!preferences->effective(feature), "battery mode leaves a high-cost feature enabled");
        preferences = lofibox::platform::host::loadRuntimePreferences(path);
        require(preferences->effective(RuntimeFeature::BatterySaver), "battery mode does not survive reopening");
        require(preferences->toggle(RuntimeFeature::BatterySaver), "cannot disable battery mode");
        require(preferences->configured() == normal, "battery mode loses individual choices");

        std::promise<bool> result;
        auto future = result.get_future();
        auto revision = preferences->revision();
        std::jthread listener([&, revision](std::stop_token stop) mutable {
            result.set_value(preferences->waitForChange(revision, stop));
        });
        require(preferences->toggle(RuntimeFeature::Spectrum), "cannot toggle spectrum");
        require(future.wait_for(1s) == std::future_status::ready && future.get(), "interface worker misses a change");
        listener.join();
        std::promise<bool> stopped;
        auto stopped_result = stopped.get_future();
        std::jthread idle([&](std::stop_token stop) {
            auto previous = preferences->revision();
            stopped.set_value(preferences->waitForChange(previous, stop));
        });
        idle.request_stop();
        require(stopped_result.wait_for(1s) == std::future_status::ready && !stopped_result.get(),
                "idle worker cannot stop without another user change");
        idle.join();

        RuntimePreferences denied{RuntimePreferences::kDefaults, [](std::uint32_t) { return false; }};
        require(!denied.toggle(RuntimeFeature::Leds) && denied.saveFailed(), "save failure not reported");
        require(denied.effective(RuntimeFeature::Leds), "failed save partially changes live settings");
        require(!std::filesystem::exists(path.string() + ".tmp"), "atomic save leaves temporary files");
        {
            std::ofstream damaged{path};
            damaged << "lofibox-preferences-v1 4294967295\n";
        }
        preferences = lofibox::platform::host::loadRuntimePreferences(path);
        require(preferences->configured() == RuntimePreferences::kDefaults, "invalid file enables unknown flags");
    } catch (const std::exception& e) {
        std::error_code ignored;
        std::filesystem::remove_all(directory, ignored);
        std::cerr << e.what() << '\n';
        return 1;
    }
    std::error_code ignored;
    std::filesystem::remove_all(directory, ignored);
}
