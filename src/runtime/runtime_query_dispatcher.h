// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>

#include "runtime/runtime_command.h"
#include "runtime/runtime_session_facade.h"
#include "runtime/runtime_snapshot.h"

namespace lofibox::runtime {

class RuntimeQueryDispatcher {
public:
    explicit RuntimeQueryDispatcher(RuntimeSessionFacade& session) noexcept;

    [[nodiscard]] RuntimeSnapshot query(const RuntimeQuery& query, std::uint64_t version) const;

private:
    RuntimeSessionFacade& session_;
};

} // namespace lofibox::runtime
