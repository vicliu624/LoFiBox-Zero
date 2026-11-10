// SPDX-License-Identifier: GPL-3.0-or-later
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>
#include <unistd.h>

#include "platform/host/system_media_key_handler.h"

int main()
{
    using namespace std::chrono_literals;
    using lofibox::app::InputEvent;
    using lofibox::app::InputKey;
    using lofibox::platform::host::SystemMediaKeyHandler;
    std::mutex mutex;
    std::condition_variable changed;
    std::vector<std::pair<int, bool>> calls;
    bool release_first = false;
    {
        SystemMediaKeyHandler handler{[&](int steps, bool mute, std::stop_token) {
            std::unique_lock lock{mutex};
            calls.emplace_back(steps, mute);
            changed.notify_all();
            if (calls.size() == 1) changed.wait(lock, [&] { return release_first; });
            return true;
        }};
        assert(!handler.handle(InputEvent{InputKey::Escape, "ESC"}));
        assert(handler.handle(InputEvent{InputKey::VolumeUp, "UP"}));
        {
            std::unique_lock lock{mutex};
            assert(changed.wait_for(lock, 2s, [&] { return calls.size() == 1; }));
        }
        const auto started = std::chrono::steady_clock::now();
        for (int i = 0; i < 5000; ++i) assert(handler.handle(InputEvent{InputKey::VolumeUp, "UP"}));
        for (int i = 0; i < 3; ++i) assert(handler.handle(InputEvent{InputKey::VolumeMute, "MUTE"}));
        auto repeated_mute = InputEvent{InputKey::VolumeMute, "MUTE"};
        repeated_mute.repeated = true;
        assert(handler.handle(repeated_mute));
        // The executor is deliberately blocked; submitting a held key must not
        // wait for it or accumulate a list of thousands of commands.
        assert(std::chrono::steady_clock::now() - started < 1s);
        {
            const std::lock_guard lock{mutex};
            assert(calls.size() == 1);
            release_first = true;
        }
        changed.notify_all();
        {
            std::unique_lock lock{mutex};
            assert(changed.wait_for(lock, 2s, [&] { return calls.size() == 2; }));
            assert(calls[0].first == 1 && !calls[0].second);
            assert(calls[1].first == 20 && calls[1].second);
        }
    }

    const auto ready = std::filesystem::temp_directory_path() / ("lofibox-media-helper-" + std::to_string(::getpid()));
    std::filesystem::remove(ready);
    auto handler = std::make_unique<SystemMediaKeyHandler>([&](int, bool, std::stop_token stop) {
        const lofibox::core::OperationCancellationScope scope{stop};
        return lofibox::platform::host::runtime_detail::captureProcessOutput("/bin/sh",
            {"-c", "printf ready > \"$1\"; sleep 60", "media-key-test", ready.string()}).has_value();
    });
    assert(handler->handle(InputEvent{InputKey::VolumeDown, "DOWN"}));
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (!std::filesystem::exists(ready) && std::chrono::steady_clock::now() < deadline) std::this_thread::sleep_for(1ms);
    assert(std::filesystem::exists(ready));
    const auto stopping = std::chrono::steady_clock::now();
    handler.reset();
    assert(std::chrono::steady_clock::now() - stopping < 1s);
    std::filesystem::remove(ready);
}
