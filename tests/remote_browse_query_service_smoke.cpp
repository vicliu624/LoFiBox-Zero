// SPDX-License-Identifier: GPL-3.0-or-later

#include <cassert>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "app/app_controller_set.h"
#include "app/remote_profile_store.h"
#include "app/runtime_services.h"
#include "application/app_service_registry.h"
#include "cache/cache_manager.h"
#include "security/credential_policy.h"

namespace {

class MemoryRemoteProfileStore final : public lofibox::app::RemoteProfileStore {
public:
    std::vector<lofibox::app::RemoteServerProfile> loadProfiles() const override { return {}; }
    bool saveProfiles(const std::vector<lofibox::app::RemoteServerProfile>&) const override { return true; }
    bool saveCredentials(const lofibox::app::RemoteServerProfile&) const override { return true; }
    bool deleteCredentials(const lofibox::security::CredentialRef&) const override { return true; }
};

class FakeRemoteSourceProvider final : public lofibox::app::RemoteSourceProvider {
public:
    bool available() const override { return true; }
    std::string displayName() const override { return "SOURCE"; }
    lofibox::app::RemoteSourceSession probe(const lofibox::app::RemoteServerProfile& profile) const override
    {
        return {true, profile.name, profile.username, "token-secret", "OK"};
    }
};

class FakeRemoteCatalogProvider final : public lofibox::app::RemoteCatalogProvider {
public:
    bool available() const override { return true; }
    std::string displayName() const override { return "CATALOG"; }

    std::vector<lofibox::app::RemoteTrack> searchTracks(
        const lofibox::app::RemoteServerProfile& profile,
        const lofibox::app::RemoteSourceSession&,
        std::string_view,
        int) const override
    {
        return {{profile.base_url + "/search.mp3", "Search Song", "Search Artist", "Search Album", "", 120}};
    }

    std::vector<lofibox::app::RemoteTrack> recentTracks(
        const lofibox::app::RemoteServerProfile&,
        const lofibox::app::RemoteSourceSession&,
        int) const override
    {
        return {};
    }

    std::vector<lofibox::app::RemoteTrack> libraryTracks(
        const lofibox::app::RemoteServerProfile& profile,
        const lofibox::app::RemoteSourceSession&,
        int) const override
    {
        return {{profile.base_url + "/library.mp3", "Library Song", "Library Artist", "Library Album", "", 121}};
    }

    std::vector<lofibox::app::RemoteCatalogNode> browse(
        const lofibox::app::RemoteServerProfile& profile,
        const lofibox::app::RemoteSourceSession&,
        const lofibox::app::RemoteCatalogNode& parent,
        int) const override
    {
        if (parent.id.empty()) {
            return {{lofibox::app::RemoteCatalogNodeKind::Tracks, "tracks", "TRACKS", "1", false, true}};
        }
        return {{lofibox::app::RemoteCatalogNodeKind::Tracks, profile.base_url + "/browse.mp3", "Browse Song", "Browse Artist", true, false, "Browse Artist", "Browse Album", 122}};
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
        stream.url = track.id;
        stream.diagnostics.source_name = profile.name;
        stream.diagnostics.resolved_url_redacted = "http://remote/[redacted]";
        stream.diagnostics.connected = true;
        stream.diagnostics.connection_status = "READY";
        stream.diagnostics.codec = "mp3";
        stream.diagnostics.bitrate_kbps = 320;
        return stream;
    }
};

} // namespace

int main()
{
    const auto cache_root = std::filesystem::temp_directory_path() / "lofibox_remote_browse_query_service_smoke";
    std::error_code ec{};
    std::filesystem::remove_all(cache_root, ec);

    auto runtime = lofibox::app::withNullRuntimeServices();
    runtime.remote.remote_profile_store = std::make_shared<MemoryRemoteProfileStore>();
    runtime.remote.remote_source_provider = std::make_shared<FakeRemoteSourceProvider>();
    runtime.remote.remote_catalog_provider = std::make_shared<FakeRemoteCatalogProvider>();
    runtime.remote.remote_stream_resolver = std::make_shared<FakeRemoteStreamResolver>();
    runtime.cache.cache_manager = std::make_shared<lofibox::cache::CacheManager>(cache_root);

    lofibox::app::AppControllerSet controllers{};
    controllers.bindServices(runtime);
    lofibox::application::AppServiceRegistry registry{controllers, runtime};

    auto profile = registry.sourceProfiles().createProfile(lofibox::app::RemoteServerKind::Emby, 0);
    profile.id = "emby-home";
    profile.credential_ref.id = "credential/emby-home";
    profile.name = "Emby";
    profile.base_url = "http://remote";
    profile.username = "vic";
    profile.password = "secret";

    const auto root = registry.remoteBrowseQueries().browseRoot(profile, 1, 10);
    assert(root.command.accepted);
    assert(root.session.available);
    assert(root.nodes.size() == 1U);
    assert(root.nodes.front().title == "TRACKS");

    const auto child = registry.remoteBrowseQueries().browseChild(profile, root.session, root.nodes.front(), 10);
    assert(child.command.accepted);
    assert(child.nodes.size() == 1U);
    assert(child.playable_tracks.size() == 1U);
    assert(child.playable_tracks.front().title == "Browse Song");

    const auto search = registry.remoteBrowseQueries().search(profile, 1, "Song", 5);
    assert(search.command.accepted);
    assert(search.tracks.size() == 1U);
    assert(search.tracks.front().source_label == "Emby");

    const auto resolve = registry.remoteBrowseQueries().resolve(profile, root.session, child.playable_tracks.front());
    assert(resolve.command.accepted);
    assert(resolve.stream.has_value());

    const auto source_diag = registry.remoteBrowseQueries().sourceDiagnostics(std::optional<lofibox::app::RemoteServerProfile>{profile}, root.session);
    assert(source_diag.source_label == "Emby");
    assert(source_diag.connection_status == "ONLINE");
    assert(source_diag.token_status == "REDACTED");
    assert(source_diag.permission == "READ ONLY");

    const auto stream_diag = registry.remoteBrowseQueries().streamDiagnostics(std::optional<lofibox::app::RemoteServerProfile>{profile}, resolve.stream);
    assert(stream_diag.resolved);
    assert(stream_diag.buffer_state == "READY");
    assert(stream_diag.codec == "mp3");

    std::filesystem::remove_all(cache_root, ec);
    return 0;
}
