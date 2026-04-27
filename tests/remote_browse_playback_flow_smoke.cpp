// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "app/app_runtime_context.h"
#include "app/runtime_services.h"
#include "remote/common/remote_source_registry.h"

namespace {

class FakeRemoteProfileStore final : public lofibox::app::RemoteProfileStore {
public:
    std::vector<lofibox::app::RemoteServerProfile> loadProfiles() const override
    {
        return {lofibox::app::RemoteServerProfile{
            lofibox::app::RemoteServerKind::DirectUrl,
            "direct",
            "Test Stream",
            "https://example.test/audio.mp3"}};
    }

    bool saveProfiles(const std::vector<lofibox::app::RemoteServerProfile>&) const override { return true; }
};

class FakeRemoteSourceProvider final : public lofibox::app::RemoteSourceProvider {
public:
    bool available() const override { return true; }
    std::string displayName() const override { return "REMOTE"; }
    lofibox::app::RemoteSourceSession probe(const lofibox::app::RemoteServerProfile& profile) const override
    {
        return {true, profile.name, "", "", "OK"};
    }
};

class FakeRemoteCatalogProvider final : public lofibox::app::RemoteCatalogProvider {
public:
    bool available() const override { return true; }
    std::string displayName() const override { return "CATALOG"; }

    std::vector<lofibox::app::RemoteTrack> searchTracks(
        const lofibox::app::RemoteServerProfile&,
        const lofibox::app::RemoteSourceSession&,
        std::string_view,
        int) const override
    {
        return {{"https://example.test/search.mp3", "Search Hit", "Remote", "", "", 0}};
    }

    std::vector<lofibox::app::RemoteTrack> recentTracks(
        const lofibox::app::RemoteServerProfile&,
        const lofibox::app::RemoteSourceSession&,
        int) const override
    {
        return {};
    }

    std::vector<lofibox::app::RemoteCatalogNode> browse(
        const lofibox::app::RemoteServerProfile& profile,
        const lofibox::app::RemoteSourceSession&,
        const lofibox::app::RemoteCatalogNode&,
        int) const override
    {
        return {{lofibox::app::RemoteCatalogNodeKind::Tracks, profile.base_url, "Playable Stream", "REMOTE", true, false}};
    }
};

class FakeRemoteStreamResolver final : public lofibox::app::RemoteStreamResolver {
public:
    bool available() const override { return true; }
    std::string displayName() const override { return "RESOLVER"; }
    std::optional<lofibox::app::ResolvedRemoteStream> resolveTrack(
        const lofibox::app::RemoteServerProfile& profile,
        const lofibox::app::RemoteSourceSession&,
        const lofibox::app::RemoteTrack& track) const override
    {
        lofibox::app::ResolvedRemoteStream stream{};
        stream.url = track.id.empty() ? profile.base_url : track.id;
        stream.seekable = true;
        stream.diagnostics.source_name = profile.name;
        stream.diagnostics.resolved_url_redacted = "https://example.test/[redacted]";
        stream.diagnostics.connected = true;
        stream.diagnostics.connection_status = "READY";
        return stream;
    }
};

class FakeAudioBackend final : public lofibox::app::AudioPlaybackBackend {
public:
    bool available() const override { return true; }
    std::string displayName() const override { return "AUDIO"; }
    bool playFile(const std::filesystem::path&, double) override { return false; }
    bool playUri(const std::string& uri, double) override
    {
        played_uri = uri;
        return true;
    }
    void stop() override {}
    bool isPlaying() override { return !played_uri.empty(); }
    bool isFinished() override { return false; }

    std::string played_uri{};
};

class AlwaysOnline final : public lofibox::app::ConnectivityProvider {
public:
    bool connected() const override { return true; }
    std::string displayName() const override { return "ONLINE"; }
};

} // namespace

int main()
{
    auto audio = std::make_shared<FakeAudioBackend>();
    lofibox::app::RuntimeServices services = lofibox::app::withNullRuntimeServices({});
    services.connectivity.provider = std::make_shared<AlwaysOnline>();
    services.remote.remote_profile_store = std::make_shared<FakeRemoteProfileStore>();
    services.remote.remote_source_provider = std::make_shared<FakeRemoteSourceProvider>();
    services.remote.remote_catalog_provider = std::make_shared<FakeRemoteCatalogProvider>();
    services.remote.remote_stream_resolver = std::make_shared<FakeRemoteStreamResolver>();
    services.playback.audio_backend = audio;

    lofibox::app::AppRuntimeContext app{{}, {}, std::move(services)};
    app.pushPage(lofibox::app::AppPage::SourceManager);
    auto source_rows = app.pageModel().rows;
    const int selected_profile_row = static_cast<int>(source_rows.size()) - 1;
    app.navigationState().list_selection.selected = selected_profile_row;
    app.confirmListPage();
    if (app.currentPage() != lofibox::app::AppPage::RemoteBrowse) {
        std::cerr << "Expected profile selection to open RemoteBrowse.\n";
        return 1;
    }

    if (app.pageModel().rows.empty() || app.pageModel().rows.front().first != "Playable Stream") {
        std::cerr << "Expected RemoteBrowse to show playable catalog node.\n";
        return 1;
    }

    app.navigationState().list_selection.selected = 0;
    app.confirmListPage();
    if (app.currentPage() != lofibox::app::AppPage::StreamDetail) {
        std::cerr << "Expected playable remote node to open StreamDetail.\n";
        return 1;
    }

    app.confirmListPage();
    if (app.currentPage() != lofibox::app::AppPage::NowPlaying) {
        std::cerr << "Expected StreamDetail confirm to enter NowPlaying.\n";
        return 1;
    }
    if (audio->played_uri != "https://example.test/audio.mp3") {
        std::cerr << "Expected audio backend to receive resolved remote URL.\n";
        return 1;
    }
    if (app.playbackSession().current_stream_title != "Playable Stream") {
        std::cerr << "Expected playback session to expose remote stream title.\n";
        return 1;
    }

    return 0;
}
