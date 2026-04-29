// SPDX-License-Identifier: GPL-3.0-or-later

#include "runtime/settings_runtime.h"

namespace lofibox::runtime {

SettingsRuntimeSnapshot SettingsRuntime::snapshot(std::uint64_t version) const noexcept
{
    SettingsRuntimeSnapshot result{};
    result.version = version;
    return result;
}

} // namespace lofibox::runtime
