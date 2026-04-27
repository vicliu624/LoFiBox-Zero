// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <iosfwd>
#include <string>
#include <vector>

namespace lofibox::targets {

bool handleCommonCliOptions(int argc, char** argv, std::ostream& out);
[[nodiscard]] std::vector<std::string> positionalOpenUris(int argc, char** argv);

} // namespace lofibox::targets
