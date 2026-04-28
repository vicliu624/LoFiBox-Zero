// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/app_page.h"
#include "app/library_controller.h"

namespace lofibox::application {

class LibraryOpenActionService {
public:
    explicit LibraryOpenActionService(app::LibraryController& controller) noexcept;

    [[nodiscard]] app::LibraryOpenResult openSelectedListItem(app::AppPage page, int selected) const;

private:
    app::LibraryController& controller_;
};

} // namespace lofibox::application
