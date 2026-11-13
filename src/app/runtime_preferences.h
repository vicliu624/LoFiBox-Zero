// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string_view>
#include <utility>

namespace lofibox::app {

enum class RuntimeFeature : std::uint32_t {
    WebUi = 1U, ExternalControl = 2U, Spectrum = 4U,
    Leds = 8U, LedAnimation = 16U, BatterySaver = 32U,
};

inline constexpr std::optional<RuntimeFeature> preferenceForRow(std::string_view name) noexcept
{
    if (name == "LONG BATTERY") return RuntimeFeature::BatterySaver;
    if (name == "SPECTRUM") return RuntimeFeature::Spectrum;
    if (name == "LEDS") return RuntimeFeature::Leds;
    if (name == "LED ANIMATION") return RuntimeFeature::LedAnimation;
    if (name == "WEBUI") return RuntimeFeature::WebUi;
    if (name == "CLI / TUI") return RuntimeFeature::ExternalControl;
    return std::nullopt;
}

// Shared across frontends and audio workers. Reads never lock or touch disk.
// Only explicit user changes persist; battery mode preserves individual choices.
class RuntimePreferences {
public:
    static constexpr std::uint32_t kDefaults = 31U;
    static constexpr std::uint32_t kValidMask = 63U;
    using Save = std::function<bool(std::uint32_t)>;

    explicit RuntimePreferences(std::uint32_t flags = kDefaults, Save save = {}, bool loaded = false)
        : flags_(flags & kValidMask), save_(std::move(save)), configured_(loaded) {}

    [[nodiscard]] std::uint32_t configured() const noexcept { return flags_.load(std::memory_order_acquire); }
    [[nodiscard]] static bool enabled(std::uint32_t flags, RuntimeFeature feature) noexcept
    {
        return (flags & static_cast<std::uint32_t>(feature)) != 0;
    }
    [[nodiscard]] static bool effective(std::uint32_t flags, RuntimeFeature feature) noexcept
    {
        return enabled(flags, feature) && (feature == RuntimeFeature::BatterySaver
            || !enabled(flags, RuntimeFeature::BatterySaver));
    }
    [[nodiscard]] bool effective(RuntimeFeature feature) const noexcept { return effective(configured(), feature); }
    [[nodiscard]] std::uint64_t revision() const noexcept { return revision_.load(std::memory_order_acquire); }
    [[nodiscard]] bool saveFailed() const noexcept { return save_failed_.load(); }

    bool toggle(RuntimeFeature feature)
    {
        {
            const std::lock_guard lock{mutex_};
            const auto next = configured() ^ static_cast<std::uint32_t>(feature);
            if (save_ && !save_(next)) { save_failed_ = true; return false; }
            save_failed_ = false;
            configured_ = true;
            flags_.store(next, std::memory_order_release);
            revision_.fetch_add(1, std::memory_order_release);
        }
        changed_.notify_all();
        return true;
    }

    // Target command-line/environment defaults seed a new profile only.
    // A saved OFF must survive the Cardputer launcher's WEBUI=1 default.
    void configureWebUiDefault(bool on)
    {
        const std::lock_guard lock{mutex_};
        if (configured_) return;
        const auto mask = static_cast<std::uint32_t>(RuntimeFeature::WebUi);
        flags_.store(on ? configured() | mask : configured() & ~mask, std::memory_order_release);
    }

    void configureLedsDefault(bool on)
    {
        const std::lock_guard lock{mutex_};
        if (configured_) return;
        const auto mask = static_cast<std::uint32_t>(RuntimeFeature::Leds);
        flags_.store(on ? configured() | mask : configured() & ~mask, std::memory_order_release);
    }

    bool waitForChange(std::uint64_t& previous, std::stop_token stop) const
    {
        std::unique_lock lock{mutex_};
        if (!changed_.wait(lock, stop, [&] { return revision() != previous; })) return false;
        previous = revision();
        return true;
    }

private:
    std::atomic<std::uint32_t> flags_;
    Save save_;
    bool configured_{};
    std::atomic_bool save_failed_{};
    std::atomic<std::uint64_t> revision_{};
    mutable std::mutex mutex_;
    mutable std::condition_variable_any changed_;
};
} // namespace lofibox::app
