// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/app_controller_set.h"
#include "app/runtime_services.h"
#include "application/app_service_registry.h"

namespace lofibox::application {

class AppServiceHost {
public:
    explicit AppServiceHost(app::RuntimeServices& services) noexcept
        : services_(services)
    {
        controllers_.bindServices(services_);
    }

    [[nodiscard]] AppServiceRegistry registry() noexcept
    {
        return AppServiceRegistry{controllers_, services_};
    }

    [[nodiscard]] AppServiceRegistry registry() const noexcept
    {
        return AppServiceRegistry{
            const_cast<app::AppControllerSet&>(controllers_),
            const_cast<app::RuntimeServices&>(services_)};
    }

    [[nodiscard]] app::AppControllerSet& controllers() noexcept
    {
        return controllers_;
    }

    [[nodiscard]] const app::AppControllerSet& controllers() const noexcept
    {
        return controllers_;
    }

    [[nodiscard]] app::RuntimeServices& services() noexcept
    {
        return services_;
    }

    [[nodiscard]] const app::RuntimeServices& services() const noexcept
    {
        return services_;
    }

private:
    app::AppControllerSet controllers_{};
    app::RuntimeServices& services_;
};

} // namespace lofibox::application
