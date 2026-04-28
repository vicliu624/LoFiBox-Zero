// SPDX-License-Identifier: GPL-3.0-or-later

#include "application/cache_command_service.h"

namespace lofibox::application {

CacheCommandService::CacheCommandService(const app::RuntimeServices& services) noexcept
    : services_(services)
{
}

CacheStatusResult CacheCommandService::status() const
{
    CacheStatusResult result{};
    if (!services_.cache.cache_manager) {
        result.command = CommandResult::failure("cache.unavailable", "CACHE UNAVAILABLE");
        return result;
    }
    result.root = services_.cache.cache_manager->root();
    result.usage = services_.cache.cache_manager->usage();
    result.command = CommandResult::success("cache.status", "OK");
    return result;
}

CacheClearResult CacheCommandService::clearAll() const
{
    CacheClearResult result{};
    if (!services_.cache.cache_manager) {
        result.command = CommandResult::failure("cache.unavailable", "CACHE UNAVAILABLE");
        return result;
    }
    result.before = services_.cache.cache_manager->usage();
    result.removed_entries = services_.cache.cache_manager->clearAll();
    result.after = services_.cache.cache_manager->usage();
    result.command = CommandResult::success("cache.clear", "CLEARED");
    return result;
}

CacheClearResult CacheCommandService::collectGarbage() const
{
    CacheClearResult result{};
    if (!services_.cache.cache_manager) {
        result.command = CommandResult::failure("cache.unavailable", "CACHE UNAVAILABLE");
        return result;
    }
    result.before = services_.cache.cache_manager->usage();
    result.removed_entries = services_.cache.cache_manager->collectGarbage();
    result.after = services_.cache.cache_manager->usage();
    result.command = CommandResult::success("cache.gc", "GC");
    return result;
}

} // namespace lofibox::application
