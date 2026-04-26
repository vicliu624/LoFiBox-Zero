// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/app_page.h"
#include "app/library_controller.h"

namespace lofibox::app {

class LibraryOpenActionResolver {
public:
    [[nodiscard]] static LibraryOpenResult openSelectedListItem(
        LibraryController& controller,
        AppPage page,
        int selected);
};

} // namespace lofibox::app
