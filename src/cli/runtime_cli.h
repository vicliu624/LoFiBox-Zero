// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <iosfwd>
#include <optional>

namespace lofibox::cli {

[[nodiscard]] std::optional<int> runRuntimeCliCommand(
    int argc,
    char** argv,
    std::ostream& out,
    std::ostream& err);

} // namespace lofibox::cli
