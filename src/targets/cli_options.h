// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <iosfwd>

namespace lofibox::targets {

bool handleCommonCliOptions(int argc, char** argv, std::ostream& out);

} // namespace lofibox::targets
