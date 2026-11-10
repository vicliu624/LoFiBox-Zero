// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <array>
#include <charconv>
#include <chrono>
#include <optional>
#include <string_view>

namespace lofibox::core {
// Strict, bounded durations shared by all command surfaces. Empty means leave
// the setting alone; OFF cancels. Timers deliberately do not survive a restart.
inline std::optional<std::chrono::seconds> parseTimerDuration(std::string_view text) noexcept
{
    if (text == "OFF") return std::chrono::seconds::zero();
    if (text.size() < 2 || text.size() > 6) return std::nullopt;
    const auto unit = text.back();
    const int multiplier = unit == 'S' ? 1 : unit == 'M' ? 60 : unit == 'H' ? 3600 : 0;
    if (!multiplier) return std::nullopt;
    text.remove_suffix(1);
    int value{};
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size()
        || value <= 0 || value > 86400 / multiplier) return std::nullopt;
    return std::chrono::seconds{value * multiplier};
}

inline constexpr std::array<std::string_view, 7> kSleepTimerChoices{
    "OFF", "15M", "30M", "45M", "60M", "90M", "120M"};
inline constexpr std::array<std::string_view, 6> kScreenTimeoutChoices{
    "OFF", "30S", "1M", "2M", "5M", "10M"};

template<std::size_t N>
inline std::string_view nextTimerChoice(std::string_view current, const std::array<std::string_view, N>& choices)
{
    for (std::size_t i = 0; i < N; ++i)
        if (choices[i] == current) return choices[(i + 1) % N];
    return choices.front();
}
} // namespace lofibox::core
