// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "runtime/runtime_command.h"
#include "runtime/runtime_result.h"
#include "runtime/runtime_snapshot.h"

namespace lofibox::runtime {

class RuntimeCommandBus;

class RuntimeCommandServer {
public:
    explicit RuntimeCommandServer(RuntimeCommandBus& bus) noexcept;

    [[nodiscard]] RuntimeCommandResult dispatch(const RuntimeCommand& command);
    [[nodiscard]] RuntimeSnapshot query(const RuntimeQuery& query) const;
    [[nodiscard]] RuntimeSnapshot snapshot() const;

private:
    RuntimeCommandBus& bus_;
};

} // namespace lofibox::runtime
