// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/runtime_provider_factories.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "platform/host/runtime_remote_media_tool.h"

namespace lofibox::platform::host {
namespace {
using namespace runtime_detail;

class PythonRemoteSourceProvider final : public app::RemoteSourceProvider {
public:
    [[nodiscard]] bool available() const override
    {
        return client_.available();
    }

    [[nodiscard]] std::string displayName() const override
    {
        return "REMOTE-TOOL";
    }

    [[nodiscard]] app::RemoteSourceSession probe(const app::RemoteServerProfile& profile) const override
    {
        return client_.probe(profile);
    }

private:
    RemoteMediaToolClient client_{};
};

class PythonRemoteCatalogProvider final : public app::RemoteCatalogProvider {
public:
    [[nodiscard]] bool available() const override
    {
        return client_.available();
    }

    [[nodiscard]] std::string displayName() const override
    {
        return "REMOTE-TOOL";
    }

    [[nodiscard]] std::vector<app::RemoteTrack> searchTracks(
        const app::RemoteServerProfile& profile,
        const app::RemoteSourceSession& session,
        std::string_view query,
        int limit) const override
    {
        return client_.searchTracks(profile, session, query, limit);
    }

    [[nodiscard]] std::vector<app::RemoteTrack> recentTracks(
        const app::RemoteServerProfile& profile,
        const app::RemoteSourceSession& session,
        int limit) const override
    {
        return client_.recentTracks(profile, session, limit);
    }

private:
    RemoteMediaToolClient client_{};
};

class PythonRemoteStreamResolver final : public app::RemoteStreamResolver {
public:
    [[nodiscard]] bool available() const override
    {
        return client_.available();
    }

    [[nodiscard]] std::string displayName() const override
    {
        return "REMOTE-TOOL";
    }

    [[nodiscard]] std::optional<app::ResolvedRemoteStream> resolveTrack(
        const app::RemoteServerProfile& profile,
        const app::RemoteSourceSession& session,
        const app::RemoteTrack& track) const override
    {
        return client_.resolveTrack(profile, session, track);
    }

private:
    RemoteMediaToolClient client_{};
};

} // namespace

std::shared_ptr<app::RemoteSourceProvider> createHostRemoteSourceProvider()
{
    return std::make_shared<PythonRemoteSourceProvider>();
}

std::shared_ptr<app::RemoteCatalogProvider> createHostRemoteCatalogProvider()
{
    return std::make_shared<PythonRemoteCatalogProvider>();
}

std::shared_ptr<app::RemoteStreamResolver> createHostRemoteStreamResolver()
{
    return std::make_shared<PythonRemoteStreamResolver>();
}

} // namespace lofibox::platform::host
