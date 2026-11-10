// SPDX-License-Identifier: GPL-3.0-or-later
#include "platform/device/framebuffer_backlight.h"
#include <cassert>
#include <fstream>

int main()
{
    namespace fs = std::filesystem;
    const auto root = fs::temp_directory_path() / ("lofibox-backlight-" + std::to_string(::getpid()));
    fs::create_directories(root);
    const auto write = [&](const char* name, const std::string& value) { std::ofstream(root / name) << value; };
    const auto level = [&] { int value{-1}; std::ifstream(root / "brightness") >> value; return value; };
    const auto fds = [] { return std::distance(fs::directory_iterator{"/proc/self/fd"}, fs::directory_iterator{}); };
    write("max_brightness", "100\n"); write("brightness", "0\n");
    const auto before = fds();
    {
        lofibox::platform::device::FramebufferBacklight light{root};
        assert(level() == 50); // Launcher handed off a dark panel.
        write("brightness", "23\n"); light.wake(); assert(level() == 23);
        write("brightness", "0\n"); light.wake(); assert(level() == 23);
        light.adjust(1); assert(level() == 33);
        light.adjust(-1000); assert(level() == 1);
        light.adjust(1000); assert(level() == 100);
        using namespace std::chrono_literals;
        const auto now = std::chrono::steady_clock::now();
        light.setIdleTimeout(30s, now);
        light.tick(now + 29s); assert(level() == 100 && !light.blanked());
        light.tick(now + 30s); assert(level() == 0 && light.blanked());
        // A blanked tick performs no repeated sysfs reads/writes.
        write("brightness", "7\n");
        light.tick(now + 31s); assert(level() == 7 && light.blanked());
        write("brightness", "0\n");
        light.wake(now + 32s); assert(level() == 100 && !light.blanked());
        light.tick(now + 61s); assert(level() == 100);
        light.tick(now + 62s); assert(level() == 0 && light.blanked());
        light.setIdleTimeout(0s, now + 63s); assert(level() == 100 && !light.blanked());
        light.tick(now + 300s); assert(level() == 100);
        for (int index = 0; index < 1000; ++index) { light.wake(); light.adjust(index % 2 ? 1 : -1); }
        write("brightness", "bad\n"); light.wake(); light.adjust(1);
        std::string text; std::ifstream(root / "brightness") >> text; assert(text == "bad");
    }
    assert(fds() == before);
    write("brightness", "37\n");
    {
        lofibox::platform::device::FramebufferBacklight light{root};
        const auto now = std::chrono::steady_clock::now();
        light.setIdleTimeout(std::chrono::seconds{1}, now);
        light.tick(now + std::chrono::seconds{1});
        assert(level() == 0);
    }
    assert(level() == 37); // Timed exit while blank restores the launcher panel.
    {
        lofibox::platform::device::FramebufferBacklight light{root};
        const auto now = std::chrono::steady_clock::now();
        light.setIdleTimeout(std::chrono::seconds{1}, now);
        light.tick(now + std::chrono::seconds{1});
        write("brightness", "19\n");
    }
    assert(level() == 19); // Do not overwrite a subsequent external adjustment.
    {
        lofibox::platform::device::FramebufferBacklight light{root};
        write("brightness", "0\n");
    }
    assert(level() == 0); // External blanking is not owned by this session.
    assert(fds() == before);
    write("max_brightness", "0\n"); write("brightness", "0\n");
    { lofibox::platform::device::FramebufferBacklight unavailable{root}; unavailable.wake(); unavailable.adjust(1); }
    assert(level() == 0);
    { lofibox::platform::device::FramebufferBacklight absent{root / "missing"}; absent.wake(); absent.adjust(1); }
    assert(fds() == before);
    fs::remove_all(root);
}
