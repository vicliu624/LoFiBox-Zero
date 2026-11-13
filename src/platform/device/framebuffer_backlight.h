// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <fcntl.h>
#include <unistd.h>

namespace lofibox::platform::device {
// Used only by the framebuffer presenter. Sysfs is touched at startup and on
// input and idle transitions, never polled by rendering or audio. No retained fd.
class FramebufferBacklight {
public:
    explicit FramebufferBacklight(std::filesystem::path directory = defaultDirectory())
        : directory_(std::move(directory))
    {
        if (directory_.empty()) return;
        const auto maximum = readLevel("max_brightness");
        if (!maximum || *maximum <= 0) return;
        maximum_ = *maximum;
        remembered_ = std::max(1, maximum_ / 2);
        wake();
    }

    ~FramebufferBacklight() noexcept
    {
        // Return a usable panel to the resident launcher after a timed exit.
        // Only undo blanking owned by this session; preserve external changes.
        if (blanked_) {
            try { wake(); } catch (...) { /* Cleanup must not prevent exit. */ }
        }
    }

    void wake(std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now())
    {
        last_activity_ = now;
        if (!maximum_) return;
        const auto current = readLevel("brightness");
        if (!current) return;
        if (*current > 0) {
            remembered_ = std::clamp(*current, 1, maximum_);
            blanked_ = false;
        } else if (setLevel(remembered_)) blanked_ = false;
    }

    void setIdleTimeout(std::chrono::seconds timeout,
                        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now())
    {
        if (timeout == idle_timeout_) return;
        idle_timeout_ = std::max(timeout, std::chrono::seconds::zero());
        last_activity_ = now;
        if (blanked_) wake(now);
    }

    void tick(std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now())
    {
        if (!maximum_ || blanked_ || idle_timeout_ == std::chrono::seconds::zero()
            || now - last_activity_ < idle_timeout_ || now < retry_after_) return;
        const auto current = readLevel("brightness");
        if (current && *current > 0) remembered_ = std::clamp(*current, 1, maximum_);
        // Do not suspend drawing if hardware blanking failed. Retry at most once
        // a second rather than attempting sysfs writes on every GUI frame.
        blanked_ = current && setLevel(0);
        retry_after_ = now + std::chrono::seconds{1};
    }

    [[nodiscard]] bool blanked() const noexcept { return blanked_; }

    void adjust(int steps)
    {
        if (!maximum_) return;
        const auto current = readLevel("brightness");
        if (!current) return;
        const auto step = std::max(1, maximum_ / 10);
        const auto level = *current > 0 ? *current : remembered_;
        // Keep brightness keys usable without accidentally leaving a black
        // screen. External power management may still blank it; input wakes it.
        setLevel(static_cast<int>(std::clamp<long long>(
            static_cast<long long>(level) + static_cast<long long>(steps) * step, 1, maximum_)));
    }

private:
    static std::filesystem::path defaultDirectory()
    {
        if (const auto* value = std::getenv("LOFIBOX_BACKLIGHT_DIR")) return value;
        return "/sys/class/backlight/backlight";
    }
    std::optional<int> readLevel(const char* name) const
    {
        const int fd = ::open((directory_ / name).c_str(), O_RDONLY | O_CLOEXEC);
        if (fd < 0) return std::nullopt;
        std::array<char, 32> bytes{};
        ssize_t count;
        do { count = ::read(fd, bytes.data(), bytes.size()); } while (count < 0 && errno == EINTR);
        ::close(fd);
        if (count <= 0 || count == static_cast<ssize_t>(bytes.size())) return std::nullopt;
        int value{};
        const auto result = std::from_chars(bytes.data(), bytes.data() + count, value);
        if (result.ec != std::errc{} || result.ptr == bytes.data() || value < 0) return std::nullopt;
        for (auto* tail = result.ptr; tail != bytes.data() + count; ++tail)
            if (*tail != '\n' && *tail != '\r' && *tail != ' ' && *tail != '\t') return std::nullopt;
        return value;
    }
    bool setLevel(int value)
    {
        const int fd = ::open((directory_ / "brightness").c_str(), O_WRONLY | O_TRUNC | O_CLOEXEC);
        if (fd < 0) return false;
        const auto bytes = std::to_string(value) + '\n';
        ssize_t written;
        do { written = ::write(fd, bytes.data(), bytes.size()); } while (written < 0 && errno == EINTR);
        ::close(fd);
        if (written != static_cast<ssize_t>(bytes.size())) return false;
        if (value > 0) remembered_ = value;
        return true;
    }
    std::filesystem::path directory_;
    int maximum_{};
    int remembered_{};
    bool blanked_{};
    std::chrono::seconds idle_timeout_{};
    std::chrono::steady_clock::time_point last_activity_{std::chrono::steady_clock::now()};
    std::chrono::steady_clock::time_point retry_after_{};
};
} // namespace lofibox::platform::device
