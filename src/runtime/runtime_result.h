// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>

#include "runtime/runtime_command.h"

namespace lofibox::runtime {

struct RuntimeCommandResult {
    bool accepted{false};
    bool applied{false};
    std::string code{};
    std::string message{};
    CommandOrigin origin{CommandOrigin::Unknown};
    std::string correlation_id{};
    std::uint64_t version_before_apply{0};
    std::uint64_t version_after_apply{0};

    [[nodiscard]] static RuntimeCommandResult reject(
        std::string code,
        std::string message,
        CommandOrigin origin,
        std::string correlation_id,
        std::uint64_t version_before_apply,
        std::uint64_t version_after_apply) noexcept;

    [[nodiscard]] static RuntimeCommandResult ok(
        std::string code,
        std::string message,
        CommandOrigin origin,
        std::string correlation_id,
        std::uint64_t version_before_apply,
        std::uint64_t version_after_apply,
        bool applied = true) noexcept;
};

} // namespace lofibox::runtime
