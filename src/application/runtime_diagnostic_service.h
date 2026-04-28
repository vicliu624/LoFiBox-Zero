// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <vector>

#include "app/remote_media_services.h"
#include "app/runtime_services.h"
#include "application/app_command_result.h"
#include "application/remote_browse_query_service.h"

namespace lofibox::application {

struct RuntimeCapabilityStatus {
    std::string name{};
    bool available{false};
    std::string detail{};
};

struct RuntimeDiagnosticSnapshot {
    CommandResult command{};
    std::vector<RuntimeCapabilityStatus> capabilities{};
    std::vector<RemoteSourceDiagnostics> sources{};
    bool degraded{false};
};

class RuntimeDiagnosticService {
public:
    explicit RuntimeDiagnosticService(const app::RuntimeServices& services) noexcept;

    [[nodiscard]] RuntimeDiagnosticSnapshot snapshot(const std::vector<app::RemoteServerProfile>& profiles = {}) const;

private:
    const app::RuntimeServices& services_;
};

} // namespace lofibox::application
