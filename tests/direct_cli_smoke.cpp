// SPDX-License-Identifier: GPL-3.0-or-later

#include <cassert>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "app/remote_profile_store.h"
#include "app/runtime_services.h"
#include "cache/cache_manager.h"
#include "cli/direct_cli.h"
#include "security/credential_policy.h"

namespace {

class MemoryRemoteProfileStore final : public lofibox::app::RemoteProfileStore {
public:
    std::vector<lofibox::app::RemoteServerProfile> loadProfiles() const override
    {
        return profiles;
    }

    bool saveProfiles(const std::vector<lofibox::app::RemoteServerProfile>& value) const override
    {
        profiles = value;
        return true;
    }

    bool saveCredentials(const lofibox::app::RemoteServerProfile& profile) const override
    {
        credentials = profile;
        for (auto& stored : profiles) {
            if (stored.id == profile.id) {
                stored.username = profile.username;
                stored.password = profile.password;
                stored.api_token = profile.api_token;
                stored.credential_ref = profile.credential_ref;
            }
        }
        return true;
    }

    bool deleteCredentials(const lofibox::security::CredentialRef& credential_ref) const override
    {
        for (auto& stored : profiles) {
            if (stored.credential_ref.id == credential_ref.id) {
                stored.password.clear();
                stored.api_token.clear();
            }
        }
        return true;
    }

    mutable std::vector<lofibox::app::RemoteServerProfile> profiles{};
    mutable lofibox::app::RemoteServerProfile credentials{};
};

class FakeRemoteSourceProvider final : public lofibox::app::RemoteSourceProvider {
public:
    bool available() const override { return true; }
    std::string displayName() const override { return "SOURCE"; }
    lofibox::app::RemoteSourceSession probe(const lofibox::app::RemoteServerProfile& profile) const override
    {
        return {!profile.base_url.empty(), profile.name, profile.username, "token", profile.base_url.empty() ? "MISSING URL" : "OK"};
    }
};

class FakeRemoteCatalogProvider final : public lofibox::app::RemoteCatalogProvider {
public:
    bool available() const override { return true; }
    std::string displayName() const override { return "CATALOG"; }
    std::vector<lofibox::app::RemoteTrack> searchTracks(const lofibox::app::RemoteServerProfile&, const lofibox::app::RemoteSourceSession&, std::string_view, int) const override { return {}; }
    std::vector<lofibox::app::RemoteTrack> recentTracks(const lofibox::app::RemoteServerProfile&, const lofibox::app::RemoteSourceSession&, int) const override { return {}; }
};

class FakeRemoteStreamResolver final : public lofibox::app::RemoteStreamResolver {
public:
    bool available() const override { return true; }
    std::string displayName() const override { return "RESOLVER"; }
    std::optional<lofibox::app::ResolvedRemoteStream> resolveTrack(const lofibox::app::RemoteServerProfile&, const lofibox::app::RemoteSourceSession&, const lofibox::app::RemoteTrack&) const override { return std::nullopt; }
};

class FakeConnectivityProvider final : public lofibox::app::ConnectivityProvider {
public:
    bool connected() const override { return true; }
    std::string displayName() const override { return "ONLINE"; }
};

std::optional<int> run(std::vector<std::string> args, lofibox::app::RuntimeServices& services, std::string& out_text, std::string& err_text)
{
    std::vector<char*> argv{};
    argv.reserve(args.size());
    for (auto& arg : args) {
        argv.push_back(arg.data());
    }
    std::ostringstream out{};
    std::ostringstream err{};
    const auto result = lofibox::cli::runDirectCliCommand(static_cast<int>(argv.size()), argv.data(), services, out, err);
    out_text = out.str();
    err_text = err.str();
    return result;
}

void touchFile(const std::filesystem::path& path)
{
    std::ofstream output(path, std::ios::binary);
    output << "test";
}

} // namespace

int main()
{
    const auto root = std::filesystem::temp_directory_path() / "lofibox_direct_cli_smoke";
    std::error_code ec{};
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root / "Artist" / "Album", ec);
    touchFile(root / "Artist" / "Album" / "Song.mp3");

    auto store = std::make_shared<MemoryRemoteProfileStore>();
    auto services = lofibox::app::withNullRuntimeServices();
    services.connectivity.provider = std::make_shared<FakeConnectivityProvider>();
    services.remote.remote_profile_store = store;
    services.remote.remote_source_provider = std::make_shared<FakeRemoteSourceProvider>();
    services.remote.remote_catalog_provider = std::make_shared<FakeRemoteCatalogProvider>();
    services.remote.remote_stream_resolver = std::make_shared<FakeRemoteStreamResolver>();
    services.cache.cache_manager = std::make_shared<lofibox::cache::CacheManager>(root / "cache");
    services.cache.cache_manager->putText(lofibox::cache::CacheBucket::Metadata, "probe", "value");

    std::string out{};
    std::string err{};
    auto code = run({"lofibox", "source", "add", "emby", "--id", "emby-home", "--name", "Emby", "--base-url", "http://remote", "--username", "vic", "--password", "secret"}, services, out, err);
    assert(code && *code == 0);
    assert(store->profiles.size() == 1U);
    assert(store->profiles.front().id == "emby-home");
    assert(store->profiles.front().password == "secret");

    code = run({"lofibox", "source", "list", "--json"}, services, out, err);
    assert(code && *code == 0);
    assert(out.find("\"id\":\"emby-home\"") != std::string::npos);

    code = run({"lofibox", "source", "show", "emby-home", "--fields", "id,readiness", "--porcelain"}, services, out, err);
    assert(code && *code == 0);
    assert(out.find("emby-home") != std::string::npos);

    code = run({"lofibox", "source", "capabilities", "emby-home", "--json"}, services, out, err);
    assert(code && *code == 0);
    assert(out.find("\"credentials\":\"supported\"") != std::string::npos);

    code = run({"lofibox", "source", "probe", "emby-home"}, services, out, err);
    assert(code && *code == 0);
    assert(out.find("ONLINE") != std::string::npos);

    code = run({"lofibox", "credentials", "list", "--json"}, services, out, err);
    assert(code && *code == 0);
    assert(out.find("\"credential_ref\":\"credential/emby-home\"") != std::string::npos);

    code = run({"lofibox", "credentials", "show-ref", "credential/emby-home", "--json"}, services, out, err);
    assert(code && *code == 0);
    assert(out.find("\"password\":\"SET\"") != std::string::npos);

    code = run({"lofibox", "credentials", "delete", "emby-home"}, services, out, err);
    assert(code && *code == 0);
    assert(store->profiles.front().password.empty());

    code = run({"lofibox", "library", "scan", root.string(), "--json"}, services, out, err);
    assert(code && *code == 0);
    assert(out.find("\"tracks\":\"1\"") != std::string::npos);

    code = run({"lofibox", "library", "list", "tracks", "--root", root.string(), "--fields", "title", "--porcelain"}, services, out, err);
    assert(code && *code == 0);
    assert(out.find("Song") != std::string::npos);

    code = run({"lofibox", "library", "query", "tracks", "--artist", "Artist", "--root", root.string(), "--json"}, services, out, err);
    assert(code && *code == 0);
    assert(out.find("\"artist\":\"Artist\"") != std::string::npos);

    code = run({"lofibox", "library", "query", "Song", "--root", root.string()}, services, out, err);
    assert(code && *code == 0);
    assert(out.find("Song") != std::string::npos);

    code = run({"lofibox", "search", "Song", "--root", root.string(), "--json"}, services, out, err);
    assert(code && *code == 0);
    assert(out.find("\"group\":\"local\"") != std::string::npos);

    code = run({"lofibox", "remote", "browse", "emby-home", "--porcelain"}, services, out, err);
    assert(code && *code == 0);
    assert(out.find("ARTISTS") != std::string::npos);

    code = run({"lofibox", "cache", "status", "--json"}, services, out, err);
    assert(code && *code == 0);
    assert(out.find("\"entries\"") != std::string::npos);

    code = run({"lofibox", "cache", "purge", "--json"}, services, out, err);
    assert(code && *code == 0);
    assert(out.find("\"status\":\"CLEARED\"") != std::string::npos);

    code = run({"lofibox", "doctor", "--json"}, services, out, err);
    assert(code.has_value());
    assert(out.find("\"capabilities\"") != std::string::npos);

    code = run({"lofibox", "play"}, services, out, err);
    assert(!code.has_value());

    std::filesystem::remove_all(root, ec);
    return 0;
}
