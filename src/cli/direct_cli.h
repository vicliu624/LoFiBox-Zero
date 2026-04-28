// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <iosfwd>
#include <optional>

#include "app/runtime_services.h"

namespace lofibox::cli {

[[nodiscard]] std::optional<int> runDirectCliCommand(
    int argc,
    char** argv,
    app::RuntimeServices& services,
    std::ostream& out,
    std::ostream& err);

} // namespace lofibox::cli
