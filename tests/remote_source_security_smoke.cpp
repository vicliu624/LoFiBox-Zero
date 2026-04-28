// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>

#include "remote/common/remote_source_registry.h"
#include "security/credential_policy.h"

int main()
{
    lofibox::remote::RemoteSourceRegistry registry{};
    if (!registry.supported(lofibox::app::RemoteServerKind::Jellyfin)
        || !registry.supported(lofibox::app::RemoteServerKind::OpenSubsonic)
        || !registry.supported(lofibox::app::RemoteServerKind::Navidrome)
        || !registry.supported(lofibox::app::RemoteServerKind::Emby)) {
        std::cerr << "Expected first-batch remote source kinds to be registered.\n";
        return 1;
    }
    if (!registry.openSubsonicCompatible(lofibox::app::RemoteServerKind::Navidrome)
        || registry.providerFamily(lofibox::app::RemoteServerKind::Navidrome) != "opensubsonic") {
        std::cerr << "Expected Navidrome to be governed as OpenSubsonic-compatible.\n";
        return 1;
    }
    const auto navidrome_manifest = registry.manifest(lofibox::app::RemoteServerKind::Navidrome);
    if (navidrome_manifest.type_id != "navidrome"
        || navidrome_manifest.family != "opensubsonic"
        || !lofibox::remote::remoteProviderHasCapability(navidrome_manifest, lofibox::remote::RemoteProviderCapability::ResolveStream)) {
        std::cerr << "Expected Navidrome manifest to declare OpenSubsonic-compatible stream resolution.\n";
        return 1;
    }
    if (lofibox::remote::remoteProviderHasCapability(navidrome_manifest, lofibox::remote::RemoteProviderCapability::WritableMetadata)) {
        std::cerr << "Expected Navidrome metadata writeback to remain local-cache-only until declared explicitly.\n";
        return 1;
    }
    if (registry.manifests().size() < 15U || !registry.supported(lofibox::app::RemoteServerKind::DirectUrl) || !registry.supported(lofibox::app::RemoteServerKind::Sftp)) {
        std::cerr << "Expected first-batch and expanded remote provider manifests.\n";
        return 1;
    }

    lofibox::app::RemoteServerProfile profile{};
    profile.kind = lofibox::app::RemoteServerKind::Navidrome;
    profile.credential_ref.id = "credential/navidrome/home";
    if (profile.credential_ref.id.empty() || !profile.tls_policy.verify_peer) {
        std::cerr << "Expected remote source profiles to carry credential refs and secure TLS defaults.\n";
        return 1;
    }

    lofibox::security::SecretRedactor redactor{};
    const auto redacted = redactor.redact("https://host/rest/stream.view?id=1&api_key=secret-token&token=abc");
    if (redacted.find("secret-token") != std::string::npos || redacted.find("token=abc") != std::string::npos) {
        std::cerr << "Expected secret redactor to remove API keys and tokens from logs.\n";
        return 1;
    }

    return 0;
}
