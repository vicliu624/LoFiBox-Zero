// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/runtime_services.h"

namespace lofibox::platform::host {

[[nodiscard]] app::RuntimeServices createHostRuntimeServices();

} // namespace lofibox::platform::host
