// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <filesystem>
#include <vector>

#include "app/remote_profile_store.h"

namespace lofibox::platform::host {

class XdgRemoteProfileStore final : public app::RemoteProfileStore {
public:
    explicit XdgRemoteProfileStore(std::filesystem::path profile_path = {});

    [[nodiscard]] std::vector<app::RemoteServerProfile> loadProfiles() const override;
    bool saveProfiles(const std::vector<app::RemoteServerProfile>& profiles) const override;
    [[nodiscard]] const std::filesystem::path& profilePath() const noexcept;

private:
    std::filesystem::path profile_path_{};
};

} // namespace lofibox::platform::host
