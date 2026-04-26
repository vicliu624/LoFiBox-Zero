// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/runtime_services.h"

namespace lofibox::app {
namespace {

class NullMetadataProvider final : public MetadataProvider {
public:
    [[nodiscard]] bool available() const override { return false; }
    [[nodiscard]] std::string displayName() const override { return "BUILT-IN"; }
    [[nodiscard]] TrackMetadata read(const std::filesystem::path&, MetadataReadMode = MetadataReadMode::AllowOnline) const override { return {}; }
};

class NullTrackIdentityProvider final : public TrackIdentityProvider {
public:
    [[nodiscard]] bool available() const override { return false; }
    [[nodiscard]] std::string displayName() const override { return "UNAVAILABLE"; }
    [[nodiscard]] TrackIdentity resolve(const std::filesystem::path&, const TrackMetadata& = {}) const override { return {}; }
};

class NullArtworkProvider final : public ArtworkProvider {
public:
    [[nodiscard]] bool available() const override { return false; }
    [[nodiscard]] std::string displayName() const override { return "BUILT-IN"; }
    [[nodiscard]] std::optional<core::Canvas> read(const std::filesystem::path&, ArtworkReadMode = ArtworkReadMode::AllowOnline) const override { return std::nullopt; }
};

class NullAudioPlaybackBackend final : public AudioPlaybackBackend {
public:
    [[nodiscard]] bool available() const override { return false; }
    [[nodiscard]] std::string displayName() const override { return "UNAVAILABLE"; }
    bool playFile(const std::filesystem::path&, double) override { return false; }
    void stop() override {}
    [[nodiscard]] bool isPlaying() override { return false; }
    [[nodiscard]] bool isFinished() override { return false; }
};

class NullLyricsProvider final : public LyricsProvider {
public:
    [[nodiscard]] bool available() const override { return false; }
    [[nodiscard]] std::string displayName() const override { return "UNAVAILABLE"; }
    [[nodiscard]] TrackLyrics fetch(const std::filesystem::path&, const TrackMetadata&) const override { return {}; }
};

class NullTagWriter final : public TagWriter {
public:
    [[nodiscard]] bool available() const override { return false; }
    [[nodiscard]] std::string displayName() const override { return "UNAVAILABLE"; }
    bool write(const std::filesystem::path&, const TagWriteRequest&) const override { return false; }
};

class NullConnectivityProvider final : public ConnectivityProvider {
public:
    [[nodiscard]] bool connected() const override { return false; }
    [[nodiscard]] std::string displayName() const override { return "SYSTEM"; }
};

class NullRemoteSourceProvider final : public RemoteSourceProvider {
public:
    [[nodiscard]] bool available() const override { return false; }
    [[nodiscard]] std::string displayName() const override { return "UNAVAILABLE"; }
    [[nodiscard]] RemoteSourceSession probe(const RemoteServerProfile&) const override { return {}; }
};

class NullRemoteCatalogProvider final : public RemoteCatalogProvider {
public:
    [[nodiscard]] bool available() const override { return false; }
    [[nodiscard]] std::string displayName() const override { return "UNAVAILABLE"; }
    [[nodiscard]] std::vector<RemoteTrack> searchTracks(const RemoteServerProfile&, const RemoteSourceSession&, std::string_view, int) const override { return {}; }
    [[nodiscard]] std::vector<RemoteTrack> recentTracks(const RemoteServerProfile&, const RemoteSourceSession&, int) const override { return {}; }
};

class NullRemoteStreamResolver final : public RemoteStreamResolver {
public:
    [[nodiscard]] bool available() const override { return false; }
    [[nodiscard]] std::string displayName() const override { return "UNAVAILABLE"; }
    [[nodiscard]] std::optional<ResolvedRemoteStream> resolveTrack(const RemoteServerProfile&, const RemoteSourceSession&, const RemoteTrack&) const override { return std::nullopt; }
};

template <typename T>
std::shared_ptr<T> makeNullShared()
{
    return std::make_shared<T>();
}

} // namespace

RuntimeServices withNullRuntimeServices(RuntimeServices services)
{
    if (!services.metadata_provider) {
        services.metadata_provider = makeNullShared<NullMetadataProvider>();
    }
    if (!services.track_identity_provider) {
        services.track_identity_provider = makeNullShared<NullTrackIdentityProvider>();
    }
    if (!services.artwork_provider) {
        services.artwork_provider = makeNullShared<NullArtworkProvider>();
    }
    if (!services.audio_playback_backend) {
        services.audio_playback_backend = makeNullShared<NullAudioPlaybackBackend>();
    }
    if (!services.lyrics_provider) {
        services.lyrics_provider = makeNullShared<NullLyricsProvider>();
    }
    if (!services.tag_writer) {
        services.tag_writer = makeNullShared<NullTagWriter>();
    }
    if (!services.connectivity_provider) {
        services.connectivity_provider = makeNullShared<NullConnectivityProvider>();
    }
    if (!services.remote_source_provider) {
        services.remote_source_provider = makeNullShared<NullRemoteSourceProvider>();
    }
    if (!services.remote_catalog_provider) {
        services.remote_catalog_provider = makeNullShared<NullRemoteCatalogProvider>();
    }
    if (!services.remote_stream_resolver) {
        services.remote_stream_resolver = makeNullShared<NullRemoteStreamResolver>();
    }
    return services;
}

} // namespace lofibox::app
