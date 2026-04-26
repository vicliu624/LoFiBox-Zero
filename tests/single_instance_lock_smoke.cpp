// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform/host/single_instance_lock.h"

#include <iostream>

int main()
{
    auto first = lofibox::platform::host::SingleInstanceLock::acquire("lofibox-zero-test-lock");
    if (!first.acquired()) {
        std::cerr << "first lock acquisition failed: " << first.message() << '\n';
        return 1;
    }

    auto second = lofibox::platform::host::SingleInstanceLock::acquire("lofibox-zero-test-lock");
    if (second.acquired()) {
        std::cerr << "second lock acquisition should have been rejected\n";
        return 1;
    }

    return 0;
}
