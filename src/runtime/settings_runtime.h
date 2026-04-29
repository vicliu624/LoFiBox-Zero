// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "runtime/runtime_snapshot.h"

namespace lofibox::runtime {

class SettingsRuntime {
public:
    [[nodiscard]] SettingsRuntimeSnapshot snapshot(std::uint64_t version) const noexcept;
};

} // namespace lofibox::runtime
