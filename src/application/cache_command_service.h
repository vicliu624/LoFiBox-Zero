// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstddef>
#include <filesystem>

#include "app/runtime_services.h"
#include "application/app_command_result.h"
#include "cache/cache_manager.h"

namespace lofibox::application {

struct CacheStatusResult {
    CommandResult command{};
    std::filesystem::path root{};
    cache::CacheUsage usage{};
};

struct CacheClearResult {
    CommandResult command{};
    cache::CacheUsage before{};
    cache::CacheUsage after{};
    std::size_t removed_entries{0};
};

class CacheCommandService {
public:
    explicit CacheCommandService(const app::RuntimeServices& services) noexcept;

    [[nodiscard]] CacheStatusResult status() const;
    [[nodiscard]] CacheClearResult clearAll() const;
    [[nodiscard]] CacheClearResult collectGarbage() const;

private:
    const app::RuntimeServices& services_;
};

} // namespace lofibox::application
