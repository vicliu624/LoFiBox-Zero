// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "app/remote_media_services.h"

namespace lofibox::app {

enum class BufferDecision {
    StartImmediately,
    WaitForMinimumBuffer,
    PauseAndRebuffer,
};

struct NetworkBufferState {
    std::chrono::milliseconds buffered_duration{0};
    std::chrono::milliseconds minimum_playable_duration{std::chrono::seconds{3}};
    std::chrono::milliseconds target_prefetch_duration{std::chrono::seconds{15}};
    bool connected{true};
    bool live{false};
    int retry_count{0};
    int max_retries{5};
};

struct StreamingRecoveryPlan {
    bool retry{false};
    bool reconnect{false};
    bool resume_from_buffer{false};
    std::chrono::milliseconds retry_delay{0};
    std::string reason{};
};

struct StreamingPrefetchPlan {
    std::optional<RemoteTrack> next_track{};
    std::vector<RemoteTrack> playlist_prefetch{};
    bool album_continuity{false};
};

struct StreamingQualityPolicy {
    StreamQualityPreference preference{StreamQualityPreference::Auto};
    bool allow_transcode{true};
    bool prefer_original{false};
    int manual_bitrate_kbps{0};
};

class StreamingPlaybackPolicy {
public:
    [[nodiscard]] BufferDecision bufferDecision(const NetworkBufferState& state) const noexcept;
    [[nodiscard]] StreamingRecoveryPlan recoveryPlan(const NetworkBufferState& state, bool server_timeout) const noexcept;
    [[nodiscard]] StreamingPrefetchPlan prefetchPlan(const std::vector<RemoteTrack>& queue, int current_index, bool album_continuity) const;
    [[nodiscard]] StreamingQualityPolicy qualityPolicy(StreamQualityPreference preference, bool low_bandwidth) const noexcept;
    [[nodiscard]] RemoteStreamDiagnostics diagnosticsFor(const ResolvedRemoteStream& stream, const RemoteServerProfile& profile, bool connected) const;
};

} // namespace lofibox::app
