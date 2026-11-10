// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/device/linux_framebuffer_platform.h"
#include "platform/device/linux_evdev_keyboard.h"
#include "platform/device/linux_framebuffer.h"
#include "platform/device/framebuffer_blitter.h"
#include "platform/device/framebuffer_backlight.h"

#include <atomic>
#include <csignal>
#include <utility>

#include <span>

namespace lofibox::platform::device {
namespace {

std::atomic_bool g_running{true};

void stopSignalHandler(int)
{
    g_running.store(false);
}

} // namespace

struct LinuxFramebufferPlatform::Impl {
    Impl(std::string framebuffer_path_in, std::string input_device_path_in)
        : framebuffer(std::move(framebuffer_path_in))
        , keyboard(std::move(input_device_path_in))
        , blitter(framebuffer.fixedInfo(), framebuffer.variableInfo(),
                  std::span{framebuffer.framebufferMemory(), framebuffer.framebufferSize()})
    {
        std::signal(SIGINT, stopSignalHandler);
        std::signal(SIGTERM, stopSignalHandler);

    }

    LinuxFramebuffer framebuffer;
    LinuxEvdevKeyboard keyboard;
    FramebufferBlitter blitter;
    FramebufferBacklight backlight;
};

LinuxFramebufferPlatform::LinuxFramebufferPlatform(std::string framebuffer_path, std::string input_device_path)
    : impl_(std::make_unique<Impl>(std::move(framebuffer_path), std::move(input_device_path)))
{
}

LinuxFramebufferPlatform::~LinuxFramebufferPlatform() = default;

bool LinuxFramebufferPlatform::pump()
{
    impl_->backlight.tick();
    if (impl_->keyboard.escHeldDuration() >= std::chrono::milliseconds{2000}) {
        g_running.store(false);
    }
    return g_running.load();
}

std::vector<app::InputEvent> LinuxFramebufferPlatform::drainInput()
{
    auto events = impl_->keyboard.drainInput(impl_->backlight.blanked());
    if (impl_->keyboard.inputActivity()) impl_->backlight.wake();
    int brightness_steps{};
    std::erase_if(events, [&](const auto& event) {
        if (event.key == app::InputKey::BrightnessUp) { ++brightness_steps; return true; }
        if (event.key == app::InputKey::BrightnessDown) { --brightness_steps; return true; }
        return false;
    });
    if (brightness_steps) impl_->backlight.adjust(brightness_steps);
    return events;
}

void LinuxFramebufferPlatform::present(const core::Canvas& canvas)
{
    (void)impl_->blitter.present(canvas);
}

void LinuxFramebufferPlatform::setIdleTimeout(std::chrono::seconds timeout)
{
    impl_->backlight.setIdleTimeout(timeout);
}

bool LinuxFramebufferPlatform::renderEnabled() const noexcept
{
    return !impl_->backlight.blanked();
}

} // namespace lofibox::platform::device
