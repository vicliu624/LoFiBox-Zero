// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "runtime/runtime_command.h"
#include "runtime/runtime_result.h"
#include "runtime/runtime_snapshot.h"

namespace lofibox::runtime {

class RuntimeCommandClient {
public:
    virtual ~RuntimeCommandClient() = default;

    [[nodiscard]] virtual RuntimeCommandResult dispatch(const RuntimeCommand& command) = 0;
    [[nodiscard]] virtual RuntimeSnapshot query(const RuntimeQuery& query) const = 0;

    [[nodiscard]] RuntimeSnapshot snapshot() const
    {
        return query(RuntimeQuery{RuntimeQueryKind::FullSnapshot});
    }
};

} // namespace lofibox::runtime
