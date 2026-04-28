// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>

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

struct RuntimeCommandPayload {
    int track_id{0};
    int queue_delta{0};
    int queue_index{0};
    double seek_seconds{0.0};
    int eq_band_index{0};
    int eq_gain_db{0};
    int eq_gain_delta{0};
    int preset_delta{0};
    bool enabled{false};
    std::string preset_name{};
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
