// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/source_manager_projection.h"

#include <cassert>

int main()
{
    lofibox::app::RemoteServerProfile profile{};
    profile.kind = lofibox::app::RemoteServerKind::Navidrome;
    profile.name = "Home Navidrome";
    profile.base_url = "http://navidrome.local";

    const auto rows = lofibox::app::buildSourceManagerRows(
        {profile},
        {lofibox::remote::remoteProviderManifest(lofibox::app::RemoteServerKind::Jellyfin),
         lofibox::remote::remoteProviderManifest(lofibox::app::RemoteServerKind::Navidrome)});

    assert(rows.size() == 6U);
    assert(rows[0].first == "LOCAL LIBRARY");
    assert(rows[3].first == "ADD Jellyfin");
    assert(rows[4].first == "ADD Navidrome");
    assert(rows[5].first == "Home Navidrome");
    assert(rows[5].second == "NEEDS CRED");
    return 0;
}
