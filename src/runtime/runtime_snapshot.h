// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace lofibox::runtime {

enum class RuntimePlaybackStatus {
    Empty,
    Paused,
    Playing,
};

struct PlaybackRuntimeSnapshot {
    RuntimePlaybackStatus status{RuntimePlaybackStatus::Empty};
    std::optional<int> current_track_id{};
    std::string title{};
    std::string source_label{};
    double elapsed_seconds{0.0};
    int duration_seconds{0};
    bool audio_active{false};
    bool shuffle_enabled{false};
    bool repeat_all{false};
    bool repeat_one{false};
    std::uint64_t version{0};
};

struct QueueRuntimeSnapshot {
    std::vector<int> active_ids{};
    int active_index{-1};
    bool shuffle_enabled{false};
    bool repeat_all{false};
    bool repeat_one{false};
    std::uint64_t version{0};
};

struct EqRuntimeSnapshot {
    std::array<int, 10> bands{};
    std::string preset_name{"FLAT"};
    bool enabled{false};
    std::uint64_t version{0};
};

struct RemoteSessionSnapshot {
    std::string profile_id{};
    std::string source_label{"REMOTE"};
    std::string connection_status{"UNKNOWN"};
    bool stream_resolved{false};
    std::string redacted_url{};
    std::string buffer_state{"UNKNOWN"};
    std::string recovery_action{"NONE"};
    int bitrate_kbps{0};
    std::string codec{};
    bool live{false};
    bool seekable{false};
    std::uint64_t version{0};
};

struct RuntimeSnapshot {
    PlaybackRuntimeSnapshot playback{};
    QueueRuntimeSnapshot queue{};
    EqRuntimeSnapshot eq{};
    RemoteSessionSnapshot remote{};
    std::uint64_t version{0};
};

} // namespace lofibox::runtime
