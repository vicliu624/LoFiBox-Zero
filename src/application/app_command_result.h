// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <string>
#include <utility>

namespace lofibox::application {

struct CommandResult {
    bool accepted{false};
    std::string code{};
    std::string summary{};

    [[nodiscard]] static CommandResult success(std::string code_value, std::string summary_value = {})
    {
        return {true, std::move(code_value), std::move(summary_value)};
    }

    [[nodiscard]] static CommandResult failure(std::string code_value, std::string summary_value = {})
    {
        return {false, std::move(code_value), std::move(summary_value)};
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return accepted;
    }
};

} // namespace lofibox::application
