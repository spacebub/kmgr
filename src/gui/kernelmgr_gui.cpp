// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <cstdio>
#include <exception>

#include "core/Configuration.h"
#include "gui/app/App.h"
#include "gui/app/Look.h"
#include "ttk/shell/Shell.h"
#include "ttk/system/Paths.h"

namespace Embedded {
    extern const unsigned char mark128[];
    extern const std::size_t mark128Size;
}

int main(int argc, char *argv[]) {
    ttk::Paths::set_executable(argc > 0 ? argv[0] : "");
    ttk::Paths::set_application("kernelmanager");

    try {
        Configuration::get();
    } catch (const std::exception &ex) {
        // NOLINTNEXTLINE(cert-err33-c): the process is leaving either way.
        std::fprintf(stderr, "Could not read the configuration: %s\n", ex.what());

        return 1;
    }

    // The palette goes in before anything is drawn.
    Look::install();

    ttk::Shell shell;
    BLImage icon;

    if (icon.read_from_data(Embedded::mark128, Embedded::mark128Size) == BL_SUCCESS) {
        shell.set_icon(icon);
    }

    shell.set_minimum_size(480, 380);

    if (!shell.start("KernelManager", 1120, 720)) {
        // NOLINTNEXTLINE(cert-err33-c): the process is leaving either way.
        std::fprintf(stderr, "KernelManager found no window, no surface or no font to draw with.\n");

        return 1;
    }

    try {
        App application(shell);

        application.run();
    } catch (const std::exception &ex) {
        // NOLINTNEXTLINE(cert-err33-c): the process is leaving either way.
        std::fprintf(stderr, "KernelManager stopped: %s\n", ex.what());

        return 1;
    }

    return 0;
}
