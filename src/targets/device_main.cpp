// SPDX-License-Identifier: GPL-3.0-or-later

#include <cstdlib>
#include <algorithm>
#include <chrono>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>

#include "app/lofibox_app_runner.h"
#include "cli/direct_cli.h"
#include "cli/runtime_cli.h"
#include "platform/host/legacy_asset_loader.h"
#include "platform/host/runtime_paths.h"
#include "platform/host/runtime_services_factory.h"
#include "platform/host/single_instance_lock.h"
#include "platform/host/system_media_key_handler.h"
#include "platform/device/linux_framebuffer_platform.h"
#include "platform/device/cardputer_rhythm_lights.h"
#include "targets/cli_options.h"
#if defined(LOFIBOX_HAVE_TUI)
#include "tui/tui_app.h"
#endif
#if defined(LOFIBOX_HAVE_WEBUI)
#include "platform/host/library_enrich_provider_adapter.h"
#include "platform/host/library_query_provider_adapter.h"
#include "webui/webui_config.h"
#include "webui/webui_runtime_adapter.h"
#include "webui/webui_server.h"
#include "targets/webui_starter.h"
#endif

namespace {

struct DeviceOptions {
    std::string framebuffer_path{"/dev/fb0"};
    std::string input_device_path{"/dev/input/by-path/platform-3f804000.i2c-event"};
};

class DevicePresenter final : public lofibox::platform::SurfacePresenter {
public:
    DevicePresenter(std::string framebuffer, std::string keyboard,
                    std::shared_ptr<lofibox::app::RuntimePreferences> preferences)
        : framebuffer_(std::move(framebuffer), keyboard)
    {
        const char* enabled = std::getenv("LOFIBOX_RHYTHM_LIGHTS");
        preferences->configureLedsDefault(enabled && std::string_view{enabled} == "1");
        lights_ = std::make_unique<lofibox::platform::device::CardputerRhythmLights>(std::move(keyboard));
    }
    void bindPlayback(const lofibox::application::AppServiceRegistry& registry)
    {
        if (!lights_) return;
        // Called and read on the GUI thread. The worker receives only a small
        // value copy, never a reference into the app/runtime state.
        playback_frame_ = [services = registry, this] {
            lofibox::app::AudioVisualizationFrame frame;
            bool playing;
            {
                const std::lock_guard lock{services.stateMutex()};
                const auto& playback = services.playbackStatus().session();
                frame = playback.visualization_frame;
                playing = playback.status == lofibox::app::PlaybackStatus::Playing && playback.audio_active;
            }
            const auto& preferences = services.runtimeServices().ui.preferences;
            using Feature = lofibox::app::RuntimeFeature;
            using Mode = lofibox::platform::device::RhythmLightMode;
            const auto mode = !preferences->effective(Feature::Leds) ? Mode::Off
                : preferences->effective(Feature::LedAnimation) ? Mode::Music : Mode::Steady;
            lights_->submit(frame, playing, mode);
        };
    }
    bool pump() override { return framebuffer_.pump(); }
    std::vector<lofibox::app::InputEvent> drainInput() override
    {
        auto events = framebuffer_.drainInput();
        std::erase_if(events, [this](const auto& event) { return system_keys_.handle(event); });
        return events;
    }
    void present(const lofibox::core::Canvas& canvas) override
    {
        if (playback_frame_) playback_frame_();
        framebuffer_.present(canvas);
    }
    void setIdleTimeout(std::chrono::seconds timeout) override
    {
        framebuffer_.setIdleTimeout(timeout);
        if (!framebuffer_.renderEnabled() && lights_) lights_->submit({}, false);
    }
    bool renderEnabled() const noexcept override { return framebuffer_.renderEnabled(); }
private:
    lofibox::platform::device::LinuxFramebufferPlatform framebuffer_;
    lofibox::platform::host::SystemMediaKeyHandler system_keys_;
    std::unique_ptr<lofibox::platform::device::CardputerRhythmLights> lights_;
    std::function<void()> playback_frame_;
};

std::string envOrDefault(const char* name, std::string fallback)
{
    if (const char* env = std::getenv(name)) {
        if (*env != '\0') {
            return env;
        }
    }

    return fallback;
}

DeviceOptions parseOptions(int argc, char** argv)
{
    DeviceOptions options{};
    options.framebuffer_path = envOrDefault("LOFIBOX_FBDEV", options.framebuffer_path);
    options.input_device_path = envOrDefault("LOFIBOX_INPUT_DEV", options.input_device_path);

    for (int index = 1; index < argc; ++index) {
        const std::string_view current{argv[index]};
        if (current == "--fbdev" && (index + 1) < argc) {
            options.framebuffer_path = argv[index + 1];
            ++index;
            continue;
        }

        if (current == "--input-dev" && (index + 1) < argc) {
            options.input_device_path = argv[index + 1];
            ++index;
        }
    }

    return options;
}

} // namespace

int main(int argc, char** argv)
{
    try {
        if (lofibox::targets::handleCommonCliOptions(argc, argv, std::cout)) {
            return 0;
        }

#if defined(LOFIBOX_HAVE_TUI)
        if (const auto tui_exit = lofibox::tui::runTuiSubcommandFromArgv(argc, argv, std::cout, std::cerr)) {
            return *tui_exit;
        }
#endif

        if (const auto runtime_cli_exit = lofibox::cli::runRuntimeCliCommand(argc, argv, std::cout, std::cerr)) {
            return *runtime_cli_exit;
        }

        auto services = lofibox::platform::host::createHostRuntimeServices();
        services.ui.framebuffer_backlight = true;
        if (const auto direct_cli_exit = lofibox::cli::runDirectCliCommand(argc, argv, services, std::cout, std::cerr)) {
            return *direct_cli_exit;
        }

        lofibox::app::AppStarter on_start;
#if defined(LOFIBOX_HAVE_WEBUI)
        lofibox::webui::WebUiConfig webui_config{};
        lofibox::webui::parseWebUiFromArgs(argc, argv, webui_config);
        on_start = lofibox::targets::makeWebUiStarter(webui_config, services);
#endif

        const DeviceOptions options = parseOptions(argc, argv);
        auto instance_lock = lofibox::platform::host::SingleInstanceLock::acquire();
        if (!instance_lock.acquired()) {
            std::cerr << "Device startup skipped: " << instance_lock.message() << '\n';
            return 2;
        }
        DevicePresenter device{
            std::move(options.framebuffer_path),
            std::move(options.input_device_path), services.ui.preferences};
        on_start = [&device, start_services = std::move(on_start), bound = false]
            (lofibox::runtime::RuntimeCommandClient& client,
             const lofibox::application::AppServiceRegistry& registry) mutable {
                if (!bound) { device.bindPlayback(registry); bound = true; }
                if (start_services) start_services(client, registry);
            };
        auto assets = lofibox::platform::host::loadLegacyAssets();
        lofibox::app::runLoFiBoxApp(
            device,
            std::move(assets),
            std::move(services),
            std::chrono::milliseconds::zero(),
            lofibox::targets::positionalOpenUris(argc, argv),
            std::move(on_start)
        );
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Device startup failed: " << ex.what() << '\n';
        return 1;
    }
}
