// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <iosfwd>
#include <optional>

namespace lofibox::tui {

[[nodiscard]] int runTuiAppFromArgv(int argc, char** argv, std::ostream& out, std::ostream& err);
[[nodiscard]] std::optional<int> runTuiSubcommandFromArgv(int argc, char** argv, std::ostream& out, std::ostream& err);

} // namespace lofibox::tui

