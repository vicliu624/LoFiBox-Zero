// SPDX-License-Identifier: GPL-3.0-or-later

#include "application/runtime_diagnostic_service.h"

#include <utility>

#include "application/source_profile_command_service.h"

namespace lofibox::application {
namespace {

void addCapability(std::vector<RuntimeCapabilityStatus>& output, std::string name, bool available, std::string detail)
{
    output.push_back(RuntimeCapabilityStatus{std::move(name), available, std::move(detail)});
}

} // namespace

RuntimeDiagnosticService::RuntimeDiagnosticService(const app::RuntimeServices& services) noexcept
    : services_(services)
{
}

RuntimeDiagnosticSnapshot RuntimeDiagnosticService::snapshot(const std::vector<app::RemoteServerProfile>& profiles) const
{
    RuntimeDiagnosticSnapshot result{};
    addCapability(result.capabilities, "connectivity", services_.connectivity.provider->connected(), services_.connectivity.provider->displayName());
    addCapability(result.capabilities, "metadata", services_.metadata.metadata_provider->available(), services_.metadata.metadata_provider->displayName());
    addCapability(result.capabilities, "track-identity", services_.metadata.track_identity_provider->available(), services_.metadata.track_identity_provider->displayName());
    addCapability(result.capabilities, "artwork", services_.metadata.artwork_provider->available(), services_.metadata.artwork_provider->displayName());
    addCapability(result.capabilities, "lyrics", services_.metadata.lyrics_provider->available(), services_.metadata.lyrics_provider->displayName());
    addCapability(result.capabilities, "tag-writer", services_.metadata.tag_writer->available(), services_.metadata.tag_writer->displayName());
    addCapability(result.capabilities, "audio-backend", services_.playback.audio_backend->available(), services_.playback.audio_backend->displayName());
    addCapability(result.capabilities, "remote-source", services_.remote.remote_source_provider->available(), services_.remote.remote_source_provider->displayName());
    addCapability(result.capabilities, "remote-catalog", services_.remote.remote_catalog_provider->available(), services_.remote.remote_catalog_provider->displayName());
    addCapability(result.capabilities, "remote-stream", services_.remote.remote_stream_resolver->available(), services_.remote.remote_stream_resolver->displayName());
    addCapability(result.capabilities, "cache", static_cast<bool>(services_.cache.cache_manager), services_.cache.cache_manager ? services_.cache.cache_manager->root().string() : "UNAVAILABLE");

    for (const auto& capability : result.capabilities) {
        if (!capability.available) {
            result.degraded = true;
            break;
        }
    }

    RemoteBrowseQueryService remote_queries{services_};
    SourceProfileCommandService source_profiles{services_};
    for (auto profile : profiles) {
        const auto probe = source_profiles.probe(profile, profiles.size());
        auto diagnostics = remote_queries.sourceDiagnostics(std::optional<app::RemoteServerProfile>{profile}, probe.session);
        if (!diagnostics.session_available) {
            result.degraded = true;
        }
        result.sources.push_back(std::move(diagnostics));
    }

    result.command = CommandResult::success(result.degraded ? "doctor.degraded" : "doctor.ok", result.degraded ? "DEGRADED" : "OK");
    return result;
}

} // namespace lofibox::application
