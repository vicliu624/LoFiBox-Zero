// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "runtime/runtime_command.h"
#include "runtime/runtime_snapshot.h"

namespace lofibox::runtime {

enum class RuntimeEventKind {
    RuntimeConnected,
    RuntimeDisconnected,
    RuntimeSnapshot,
    PlaybackChanged,
    PlaybackProgress,
    PlaybackError,
    QueueChanged,
    EqChanged,
    RemoteChanged,
    SettingsChanged,
    LyricsChanged,
    LyricsLineChanged,
    VisualizationFrame,
    LibraryScanStarted,
    LibraryScanProgress,
    LibraryScanCompleted,
    DiagnosticsChanged,
    CreatorChanged,
};

struct RuntimeEvent {
    RuntimeEventKind kind{RuntimeEventKind::RuntimeSnapshot};
    std::uint64_t version{0};
    std::uint64_t timestamp_ms{0};
    RuntimeSnapshot snapshot{};
    double elapsed_seconds{0.0};
    int duration_seconds{0};
    int current_index{-1};
    double offset_seconds{0.0};
    std::string text{};
    std::string code{};
    std::string message{};
};

struct RuntimeEventStreamRequest {
    RuntimeQuery query{RuntimeQueryKind::FullSnapshot, CommandOrigin::Automation, "event-stream"};
    std::uint64_t after_version{0};
    int heartbeat_ms{1000};
    int max_events{0};
};

[[nodiscard]] std::string runtimeEventKindName(RuntimeEventKind kind) noexcept;
[[nodiscard]] std::optional<RuntimeEventKind> runtimeEventKindFromName(std::string_view name) noexcept;
[[nodiscard]] std::uint64_t runtimeEventTimestampMs() noexcept;
[[nodiscard]] std::vector<RuntimeEvent> runtimeEventsBetween(
    const std::optional<RuntimeSnapshot>& previous,
    const RuntimeSnapshot& next,
    std::uint64_t timestamp_ms);

} // namespace lofibox::runtime
