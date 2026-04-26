// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <chrono>

#include "app/app_page.h"
#include "app/library_model.h"

namespace lofibox::app {

class AppLifecycleTarget {
public:
    virtual ~AppLifecycleTarget() = default;

    [[nodiscard]] virtual std::chrono::steady_clock::time_point lastUpdate() const noexcept = 0;
    virtual void setLastUpdate(std::chrono::steady_clock::time_point value) noexcept = 0;
    virtual void refreshRuntimeStatusIfDue() = 0;

    [[nodiscard]] virtual LibraryIndexState libraryState() const noexcept = 0;
    virtual void startLibraryLoading() = 0;
    virtual void refreshLibrary() = 0;

    [[nodiscard]] virtual AppPage currentPage() const noexcept = 0;
    [[nodiscard]] virtual bool bootAnimationComplete() const = 0;
    virtual void showMainMenu() = 0;

    virtual void updatePlayback(double delta_seconds) = 0;
};

void updateAppLifecycle(AppLifecycleTarget& target);

} // namespace lofibox::app
