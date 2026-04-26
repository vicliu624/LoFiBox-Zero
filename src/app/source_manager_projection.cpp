// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/source_manager_projection.h"

namespace lofibox::app {
namespace {

std::string kindLabel(RemoteServerKind kind)
{
    switch (kind) {
    case RemoteServerKind::Jellyfin: return "JELLYFIN";
    case RemoteServerKind::OpenSubsonic: return "OPENSUBSONIC";
    case RemoteServerKind::Navidrome: return "NAVIDROME";
    case RemoteServerKind::Emby: return "EMBY";
    }
    return "REMOTE";
}

} // namespace

std::vector<std::pair<std::string, std::string>> buildSourceManagerRows(const std::vector<RemoteServerProfile>& profiles)
{
    return buildSourceManagerRows(profiles, {});
}

std::vector<std::pair<std::string, std::string>> buildSourceManagerRows(
    const std::vector<RemoteServerProfile>& profiles,
    const std::vector<remote::RemoteProviderManifest>& manifests)
{
    std::vector<std::pair<std::string, std::string>> rows{};
    rows.emplace_back("LOCAL LIBRARY", "FILES");
    rows.emplace_back("DIRECT URL", "STREAM");
    rows.emplace_back("RADIO", "STATIONS");
    for (const auto& manifest : manifests) {
        rows.emplace_back("ADD " + manifest.display_name, manifest.family);
    }
    for (const auto& profile : profiles) {
        rows.emplace_back(profile.name.empty() ? kindLabel(profile.kind) : profile.name, kindLabel(profile.kind));
    }
    return rows;
}

} // namespace lofibox::app
