// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/xdg_remote_profile_store.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "platform/host/runtime_host_internal.h"
#include "platform/host/runtime_paths.h"

namespace fs = std::filesystem;

namespace lofibox::platform::host {
namespace {

std::string field(const app::RemoteServerProfile& profile, std::string_view key)
{
    if (key == "kind") return app::remoteServerKindToString(profile.kind);
    if (key == "id") return profile.id;
    if (key == "name") return profile.name;
    if (key == "base_url") return profile.base_url;
    if (key == "username") return profile.username;
    if (key == "credential_ref") return profile.credential_ref.id;
    if (key == "verify_peer") return profile.tls_policy.verify_peer ? "true" : "false";
    if (key == "allow_self_signed") return profile.tls_policy.allow_self_signed ? "true" : "false";
    return {};
}

std::string lineForProfile(const app::RemoteServerProfile& profile)
{
    const auto safe = app::sanitizeRemoteProfileForPersistence(profile);
    std::ostringstream out;
    const char separator = '\t';
    out << runtime_detail::jsonEscape(field(safe, "kind")) << separator
        << runtime_detail::jsonEscape(field(safe, "id")) << separator
        << runtime_detail::jsonEscape(field(safe, "name")) << separator
        << runtime_detail::jsonEscape(field(safe, "base_url")) << separator
        << runtime_detail::jsonEscape(field(safe, "username")) << separator
        << runtime_detail::jsonEscape(field(safe, "credential_ref")) << separator
        << field(safe, "verify_peer") << separator
        << field(safe, "allow_self_signed");
    return out.str();
}

std::vector<std::string> splitTabLine(const std::string& line)
{
    std::vector<std::string> values{};
    std::size_t begin = 0;
    while (begin <= line.size()) {
        const auto end = line.find('\t', begin);
        values.push_back(end == std::string::npos ? line.substr(begin) : line.substr(begin, end - begin));
        if (end == std::string::npos) {
            break;
        }
        begin = end + 1;
    }
    return values;
}

std::string unescapeStoredValue(const std::string& value)
{
    std::string quoted = "\"" + value + "\"";
    if (auto parsed = runtime_detail::parseJsonStringAt(quoted, 0)) {
        return *parsed;
    }
    return value;
}

} // namespace

XdgRemoteProfileStore::XdgRemoteProfileStore(std::filesystem::path profile_path)
    : profile_path_(std::move(profile_path))
{
    if (profile_path_.empty()) {
        profile_path_ = runtime_paths::appDataDir() / "remote-profiles.tsv";
    }
}

std::vector<app::RemoteServerProfile> XdgRemoteProfileStore::loadProfiles() const
{
    std::vector<app::RemoteServerProfile> profiles{};
    std::ifstream input(profile_path_, std::ios::binary);
    if (!input) {
        return profiles;
    }

    std::string line{};
    while (std::getline(input, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        const auto values = splitTabLine(line);
        if (values.size() < 8U) {
            continue;
        }
        app::RemoteServerProfile profile{};
        profile.kind = app::remoteServerKindFromString(unescapeStoredValue(values[0]));
        profile.id = unescapeStoredValue(values[1]);
        profile.name = unescapeStoredValue(values[2]);
        profile.base_url = unescapeStoredValue(values[3]);
        profile.username = unescapeStoredValue(values[4]);
        profile.credential_ref.id = unescapeStoredValue(values[5]);
        profile.tls_policy.verify_peer = values[6] != "false";
        profile.tls_policy.allow_self_signed = values[7] == "true";
        profiles.push_back(std::move(profile));
    }
    return profiles;
}

bool XdgRemoteProfileStore::saveProfiles(const std::vector<app::RemoteServerProfile>& profiles) const
{
    std::error_code ec{};
    fs::create_directories(profile_path_.parent_path(), ec);
    if (ec) {
        return false;
    }
    std::ofstream output(profile_path_, std::ios::binary | std::ios::trunc);
    if (!output) {
        return false;
    }
    output << "# LoFiBox remote profiles v1\n";
    for (const auto& profile : profiles) {
        output << lineForProfile(profile) << '\n';
    }
    return static_cast<bool>(output);
}

const std::filesystem::path& XdgRemoteProfileStore::profilePath() const noexcept
{
    return profile_path_;
}

} // namespace lofibox::platform::host
