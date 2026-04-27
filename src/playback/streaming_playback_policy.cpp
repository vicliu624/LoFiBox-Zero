// SPDX-License-Identifier: GPL-3.0-or-later

#include "playback/streaming_playback_policy.h"

#include <algorithm>

#include "security/credential_policy.h"

namespace lofibox::app {

BufferDecision StreamingPlaybackPolicy::bufferDecision(const NetworkBufferState& state) const noexcept
{
    if (!state.connected && !state.live) {
        return BufferDecision::PauseAndRebuffer;
    }
    if (state.live) {
        return state.connected ? BufferDecision::StartImmediately : BufferDecision::PauseAndRebuffer;
    }
    return state.buffered_duration >= state.minimum_playable_duration
        ? BufferDecision::StartImmediately
        : BufferDecision::WaitForMinimumBuffer;
}

StreamingRecoveryPlan StreamingPlaybackPolicy::recoveryPlan(const NetworkBufferState& state, bool server_timeout) const noexcept
{
    StreamingRecoveryPlan plan{};
    if (state.connected && !server_timeout) {
        return plan;
    }
    plan.retry = state.retry_count < state.max_retries;
    plan.reconnect = server_timeout || state.live;
    plan.resume_from_buffer = state.buffered_duration > std::chrono::seconds{1};
    plan.retry_delay = std::chrono::milliseconds{std::min(5000, 250 * (state.retry_count + 1))};
    plan.reason = server_timeout ? "SERVER TIMEOUT" : "NETWORK INTERRUPTED";
    return plan;
}

StreamingPrefetchPlan StreamingPlaybackPolicy::prefetchPlan(const std::vector<RemoteTrack>& queue, int current_index, bool album_continuity) const
{
    StreamingPrefetchPlan plan{};
    if (current_index + 1 >= 0 && current_index + 1 < static_cast<int>(queue.size())) {
        plan.next_track = queue[static_cast<std::size_t>(current_index + 1)];
    }
    plan.album_continuity = album_continuity;
    if (album_continuity) {
        for (int index = current_index + 1; index < static_cast<int>(queue.size()) && static_cast<int>(plan.playlist_prefetch.size()) < 3; ++index) {
            plan.playlist_prefetch.push_back(queue[static_cast<std::size_t>(index)]);
        }
    }
    return plan;
}

StreamingQualityPolicy StreamingPlaybackPolicy::qualityPolicy(StreamQualityPreference preference, bool low_bandwidth) const noexcept
{
    StreamingQualityPolicy policy{};
    policy.preference = low_bandwidth ? StreamQualityPreference::LowBandwidth : preference;
    policy.prefer_original = policy.preference == StreamQualityPreference::Original;
    policy.allow_transcode = !policy.prefer_original;
    policy.manual_bitrate_kbps = policy.preference == StreamQualityPreference::LowBandwidth ? 128 : 0;
    return policy;
}

RemoteStreamDiagnostics StreamingPlaybackPolicy::diagnosticsFor(const ResolvedRemoteStream& stream, const RemoteServerProfile& profile, bool connected) const
{
    RemoteStreamDiagnostics diagnostics = stream.diagnostics;
    diagnostics.source_name = profile.name.empty() ? profile.base_url : profile.name;
    diagnostics.resolved_url_redacted = security::SecretRedactor{}.redact(stream.url);
    diagnostics.seekable = stream.seekable;
    diagnostics.connected = connected;
    diagnostics.connection_status = connected ? "CONNECTED" : "DISCONNECTED";
    return diagnostics;
}

} // namespace lofibox::app
