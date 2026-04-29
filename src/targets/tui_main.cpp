// SPDX-License-Identifier: GPL-3.0-or-later

#include <exception>
#include <iostream>

#include "tui/tui_app.h"

int main(int argc, char** argv)
{
    try {
        return lofibox::tui::runTuiAppFromArgv(argc, argv, std::cout, std::cerr);
    } catch (const std::exception& ex) {
        std::cerr << "TUI startup failed: " << ex.what() << '\n';
        return 1;
    }
}

