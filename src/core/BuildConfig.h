// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_BUILDCONFIG_H
#define KERNELMGR_BUILDCONFIG_H


#include <optional>
#include <string>

#include "Kernel.h"

// Kept inside the build directory, so deleting or unpacking it again clears it.
namespace BuildConfig {
    struct Choices {
        std::string config;
        std::string patch;
        Toolchain compiler = Toolchain::Unknown;
        bool force = false;
    };

    std::optional<Choices> read(const Kernel::Version &version);

    bool write(const Kernel::Version &version, const Choices &choices);
}


#endif //KERNELMGR_BUILDCONFIG_H
