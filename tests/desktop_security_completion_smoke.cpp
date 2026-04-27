// SPDX-License-Identifier: GPL-3.0-or-later

#include "desktop/desktop_integration_boundary.h"
#include "security/credential_policy.h"

#include <cassert>

int main()
{
    const auto desktop = lofibox::desktop::buildDesktopRuntimeIntegrationState(
        true,
        true,
        true,
        {"file:///music/a.mp3", "https://example.test/a.mp3"});
    assert(desktop.dbus_available);
    assert(desktop.media_keys_available);
    assert(desktop.notifications_available);
    assert(desktop.pending_open_request.uris.size() == 2);

    lofibox::security::CredentialStore store{};
    lofibox::security::CredentialRecord record{};
    record.ref.id = "server-1";
    record.service = "emby";
    record.username = "user";
    record.token = "token";
    record.permissions = {
        lofibox::security::CredentialPermission::ReadCatalog,
        lofibox::security::CredentialPermission::ResolveStream,
        lofibox::security::CredentialPermission::WriteFavorite,
    };
    assert(store.save(record));
    const auto status = store.status(record.ref);
    assert(status.state == lofibox::security::CredentialLifecycleState::Valid);
    assert(status.writable);
    assert(lofibox::security::credentialPermissionLabel(lofibox::security::CredentialPermission::ResolveStream) == "RESOLVE STREAM");
    assert(store.revoke(record.ref));
    assert(store.status(record.ref).state == lofibox::security::CredentialLifecycleState::Revoked);
    return 0;
}
