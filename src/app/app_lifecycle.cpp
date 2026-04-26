// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/app_lifecycle.h"

namespace lofibox::app {

void updateAppLifecycle(AppLifecycleTarget& target)
{
    const auto now = std::chrono::steady_clock::now();
    const double delta = std::chrono::duration<double>(now - target.lastUpdate()).count();
    target.setLastUpdate(now);
    target.refreshRuntimeStatusIfDue();

    if (target.libraryState() == LibraryIndexState::Uninitialized) {
        target.startLibraryLoading();
        return;
    }

    if (target.libraryState() == LibraryIndexState::Loading) {
        target.refreshLibrary();
    }

    if (target.currentPage() == AppPage::Boot
        && target.libraryState() != LibraryIndexState::Uninitialized
        && target.libraryState() != LibraryIndexState::Loading
        && target.bootAnimationComplete()) {
        target.showMainMenu();
    }

    target.updatePlayback(delta);
}

} // namespace lofibox::app
