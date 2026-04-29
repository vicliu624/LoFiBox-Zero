// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "runtime/runtime_command_client.h"

namespace lofibox::runtime {

class RuntimeCommandServer;

class InProcessRuntimeCommandClient final : public RuntimeCommandClient {
public:
    explicit InProcessRuntimeCommandClient(RuntimeCommandServer& server) noexcept;

    [[nodiscard]] RuntimeCommandResult dispatch(const RuntimeCommand& command) override;
    [[nodiscard]] RuntimeSnapshot query(const RuntimeQuery& query) const override;

private:
    RuntimeCommandServer& server_;
};

} // namespace lofibox::runtime
