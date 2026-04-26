// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/xdg_remote_profile_store.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

int main()
{
    const auto root = std::filesystem::temp_directory_path() / "lofibox_remote_profile_store_smoke";
    std::error_code ec{};
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);

    const auto path = root / "remote-profiles.tsv";
    lofibox::platform::host::XdgRemoteProfileStore store{path};

    lofibox::app::RemoteServerProfile profile{};
    profile.kind = lofibox::app::RemoteServerKind::Navidrome;
    profile.id = "home-nav";
    profile.name = "Home Navidrome";
    profile.base_url = "https://music.example";
    profile.username = "vic";
    profile.password = "must-not-persist";
    profile.api_token = "must-not-persist-token";
    profile.credential_ref.id = "secret/home-nav";
    profile.tls_policy.verify_peer = true;

    assert(store.saveProfiles({profile}));
    const auto loaded = store.loadProfiles();
    assert(loaded.size() == 1U);
    assert(loaded.front().kind == lofibox::app::RemoteServerKind::Navidrome);
    assert(loaded.front().credential_ref.id == "secret/home-nav");
    assert(loaded.front().password.empty());
    assert(loaded.front().api_token.empty());

    std::ifstream input(path, std::ios::binary);
    const std::string content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    assert(content.find("must-not-persist") == std::string::npos);
    assert(content.find("must-not-persist-token") == std::string::npos);

    std::filesystem::remove_all(root, ec);
    return 0;
}
