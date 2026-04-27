// SPDX-License-Identifier: GPL-3.0-or-later

#include "playback/streaming_playback_policy.h"
#include "remote/common/remote_catalog_model.h"
#include "remote/common/remote_source_registry.h"
#include "remote/common/stream_source_model.h"

#include <cassert>

int main()
{
    lofibox::remote::RemoteSourceRegistry registry{};
    assert(registry.supported(lofibox::app::RemoteServerKind::Jellyfin));
    assert(registry.supported(lofibox::app::RemoteServerKind::Smb));
    assert(registry.supported(lofibox::app::RemoteServerKind::DlnaUpnp));
    assert(registry.manifests().size() >= 15);

    const auto root_nodes = lofibox::remote::RemoteCatalogMap::rootNodes();
    assert(root_nodes.size() >= 10);
    assert(lofibox::remote::RemoteCatalogMap::explainNode(lofibox::app::RemoteCatalogNodeKind::Artists).find("artists") != std::string::npos);

    auto hls = lofibox::remote::StreamSourceClassifier::classify("https://example.test/live.m3u8");
    assert(hls.adaptive);
    assert(hls.protocol == lofibox::app::StreamProtocol::Hls);
    auto smb = lofibox::remote::StreamSourceClassifier::classify("smb://server/music/a.flac");
    assert(smb.protocol == lofibox::app::StreamProtocol::Smb);
    assert(lofibox::remote::StreamSourceClassifier::redactedUri("https://user:pass@example.test/a.mp3").find("***:***@") != std::string::npos);

    lofibox::app::StreamingPlaybackPolicy policy{};
    lofibox::app::NetworkBufferState buffer{};
    buffer.buffered_duration = std::chrono::seconds{1};
    assert(policy.bufferDecision(buffer) == lofibox::app::BufferDecision::WaitForMinimumBuffer);
    buffer.buffered_duration = std::chrono::seconds{5};
    assert(policy.bufferDecision(buffer) == lofibox::app::BufferDecision::StartImmediately);
    buffer.connected = false;
    const auto recovery = policy.recoveryPlan(buffer, true);
    assert(recovery.retry);
    assert(recovery.reconnect);

    std::vector<lofibox::app::RemoteTrack> queue{{"1", "A"}, {"2", "B"}, {"3", "C"}};
    const auto prefetch = policy.prefetchPlan(queue, 0, true);
    assert(prefetch.next_track);
    assert(prefetch.playlist_prefetch.size() == 2);

    const auto quality = policy.qualityPolicy(lofibox::app::StreamQualityPreference::Original, false);
    assert(quality.prefer_original);
    assert(!quality.allow_transcode);
    return 0;
}
