// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <string>

namespace lofibox::runtime {

struct RuntimeCommandResult {
    bool accepted{false};
    bool applied{false};
    std::string code{};
    std::string message{};
    std::string correlation_id{};
    std::uint64_t version_after_apply{0};

    [[nodiscard]] static RuntimeCommandResult reject(
        std::string code,
        std::string message,
        std::string correlation_id,
        std::uint64_t version) noexcept;

    [[nodiscard]] static RuntimeCommandResult ok(
        std::string code,
        std::string message,
        std::string correlation_id,
        std::uint64_t version,
        bool applied = true) noexcept;
};

} // namespace lofibox::runtime
