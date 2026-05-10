// SPDX-License-Identifier: GPL-3.0-or-later

#include "groove/groove_commands.h"

namespace lofibox::groove {

PocketGrooveCommand makeGrooveCommand(PocketGrooveCommandType type) noexcept
{
    PocketGrooveCommand command{};
    command.type = type;
    return command;
}

} // namespace lofibox::groove
