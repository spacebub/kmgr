// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2024
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_CONFIGURATION_H
#define KERNELMGR_CONFIGURATION_H


#include <optional>
#include <string>

#include "Toolchain.h"

struct Settings {
    std::string bootDirectory;
    std::string grubDirectory;
    std::string archiveFormat;
    std::string kernelCdn;
    std::string baseDirectory;
    std::string archiveDirectory;
    std::string elevationCommand;
    int jobs;
    bool colors;

    Toolchain compiler;

    // customFlags runs inside elevated steps, trusted as far as elevationCommand is.
    std::string customToolchainName;
    std::string customFlags;

    // "system", "light" or "dark".
    std::string theme;

    // Unset means the user was never asked, which the frontend asks on first launch.
    std::optional<bool> notifications;
};

namespace Configuration {
    void save();

    const Settings* get();
    void set(const Settings &settings);
}


#endif //KERNELMGR_CONFIGURATION_H
