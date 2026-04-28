// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <string_view>

#include "app/remote_media_services.h"
#include "app/runtime_services.h"
#include "application/app_command_result.h"

namespace lofibox::application {

struct CredentialStatus {
    std::string ref_id{};
    bool attached{false};
    bool has_username{false};
    bool has_password{false};
    bool has_api_token{false};
};

struct CredentialSecretPatch {
    std::string username{};
    std::string password{};
    std::string api_token{};
};

class CredentialCommandService {
public:
    explicit CredentialCommandService(const app::RuntimeServices& services) noexcept;

    [[nodiscard]] CredentialStatus status(const app::RemoteServerProfile& profile) const;
    [[nodiscard]] CommandResult setSecret(app::RemoteServerProfile& profile, const CredentialSecretPatch& patch) const;
    [[nodiscard]] CommandResult deleteSecret(app::RemoteServerProfile& profile) const;

private:
    const app::RuntimeServices& services_;
};

} // namespace lofibox::application
