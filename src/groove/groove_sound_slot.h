// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace lofibox::groove {

enum class GrooveSoundType {
    Empty,
    BuiltinSample,
    UserSample,
    CapturedFromTrack,
    RecordedInput
};

enum class GroovePlaybackMode {
    OneShot,
    Gate,
    Loop,
    Slice
};

struct SampleSlice {
    std::string id{};
    std::string name{};

    double startSeconds{0.0};
    double endSeconds{0.0};

    std::int8_t pitchSemitone{0};
    float gain{1.0f};
};

struct GrooveSoundSlot {
    GrooveSoundType type{GrooveSoundType::Empty};

    std::string id{};
    std::string name{};
    std::string sourceUri{};

    GroovePlaybackMode mode{GroovePlaybackMode::OneShot};

    float gain{1.0f};
    float pitchSemitone{0.0f};
    float pan{0.0f};

    double startSeconds{0.0};
    double endSeconds{0.0};

    double fadeInMs{0.0};
    double fadeOutMs{4.0};

    bool normalized{false};
    std::uint8_t chokeGroup{0};

    std::vector<SampleSlice> slices{};
};

[[nodiscard]] const char* toString(GrooveSoundType type) noexcept;
[[nodiscard]] const char* toString(GroovePlaybackMode mode) noexcept;
[[nodiscard]] GrooveSoundType grooveSoundTypeFromString(std::string_view value) noexcept;
[[nodiscard]] GroovePlaybackMode groovePlaybackModeFromString(std::string_view value) noexcept;

} // namespace lofibox::groove
