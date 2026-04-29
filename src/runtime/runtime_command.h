// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <utility>
#include <variant>

namespace lofibox::runtime {

enum class CommandOrigin {
    Gui,
    DirectTest,
    Desktop,
    RuntimeCli,
    Automation,
    Unknown,
};

enum class RuntimeCommandKind {
    PlaybackPlay,
    PlaybackPause,
    PlaybackResume,
    PlaybackToggle,
    PlaybackStartTrack,
    PlaybackStop,
    PlaybackSeek,
    QueueStep,
    QueueJump,
    QueueClear,
    PlaybackToggleShuffle,
    PlaybackCycleRepeat,
    PlaybackCycleMainMenuMode,
    PlaybackSetRepeatAll,
    PlaybackSetRepeatOne,
    RemoteStartActiveStream,
    EqEnable,
    EqDisable,
    EqSetBand,
    EqAdjustBand,
    EqApplyPreset,
    EqCyclePreset,
    EqReset,
    RemoteReconnect,
    SettingsApplyLive,
    RuntimeShutdown,
    RuntimeReload,
};

enum class RuntimeQueryKind {
    PlaybackSnapshot,
    QueueSnapshot,
    EqSnapshot,
    RemoteSessionSnapshot,
    FullSnapshot,
};

struct EmptyPayload {};

struct PlaybackStartTrackPayload {
    int track_id{0};
};

struct PlaybackSeekPayload {
    double seconds{0.0};
};

struct QueueStepPayload {
    int delta{0};
};

struct QueueIndexPayload {
    int queue_index{0};
};

struct RuntimeEnabledPayload {
    bool enabled{false};
};

struct EqSetBandPayload {
    int eq_band_index{0};
    int eq_gain_db{0};
};

struct EqAdjustBandPayload {
    int eq_band_index{0};
    int eq_gain_delta{0};
};

struct EqCyclePresetPayload {
    int preset_delta{0};
};

struct EqApplyPresetPayload {
    std::string preset_name{};
};

using RuntimeCommandData = std::variant<
    EmptyPayload,
    PlaybackStartTrackPayload,
    PlaybackSeekPayload,
    QueueStepPayload,
    QueueIndexPayload,
    RuntimeEnabledPayload,
    EqSetBandPayload,
    EqAdjustBandPayload,
    EqCyclePresetPayload,
    EqApplyPresetPayload>;

struct RuntimeCommandPayload {
    RuntimeCommandData data{EmptyPayload{}};

    [[nodiscard]] static RuntimeCommandPayload empty() { return {}; }
    [[nodiscard]] static RuntimeCommandPayload startTrack(int track_id) { return {{PlaybackStartTrackPayload{track_id}}}; }
    [[nodiscard]] static RuntimeCommandPayload seek(double seconds) { return {{PlaybackSeekPayload{seconds}}}; }
    [[nodiscard]] static RuntimeCommandPayload queueStep(int delta) { return {{QueueStepPayload{delta}}}; }
    [[nodiscard]] static RuntimeCommandPayload queueIndex(int queue_index) { return {{QueueIndexPayload{queue_index}}}; }
    [[nodiscard]] static RuntimeCommandPayload enabled(bool value) { return {{RuntimeEnabledPayload{value}}}; }
    [[nodiscard]] static RuntimeCommandPayload eqSetBand(int band_index, int gain_db) { return {{EqSetBandPayload{band_index, gain_db}}}; }
    [[nodiscard]] static RuntimeCommandPayload eqAdjustBand(int band_index, int delta_db) { return {{EqAdjustBandPayload{band_index, delta_db}}}; }
    [[nodiscard]] static RuntimeCommandPayload eqCyclePreset(int delta) { return {{EqCyclePresetPayload{delta}}}; }
    [[nodiscard]] static RuntimeCommandPayload eqApplyPreset(std::string preset_name) { return {{EqApplyPresetPayload{std::move(preset_name)}}}; }

    template <typename Payload>
    [[nodiscard]] const Payload* get() const noexcept
    {
        return std::get_if<Payload>(&data);
    }
};

struct RuntimeCommand {
    RuntimeCommandKind kind{RuntimeCommandKind::PlaybackToggle};
    RuntimeCommandPayload payload{};
    CommandOrigin origin{CommandOrigin::Unknown};
    std::string correlation_id{};
};

struct RuntimeQuery {
    RuntimeQueryKind kind{RuntimeQueryKind::FullSnapshot};
    CommandOrigin origin{CommandOrigin::Unknown};
    std::string correlation_id{};
};

} // namespace lofibox::runtime
