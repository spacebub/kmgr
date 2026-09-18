// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_WORKFLOWFACTORY_H
#define KERNELMGR_WORKFLOWFACTORY_H


#include <string>
#include "Workflow.h"
#include "core/Toolchain.h"

struct Options {
    enum Stage {
        NONE = 0,
        PULL = 1 << 0,
        DOWNLOAD = 1 << 1,
        EXTRACT = 1 << 2,
        PATCH = 1 << 3,
        CONFIGURE = 1 << 4,
        BUILD = 1 << 5,
        INSTALL = 1 << 6,
        SIGN = 1 << 7,
        CLEAN_INSTALLED = 1 << 8,
        CLEAN_SOURCE = 1 << 9,
        CLEAN_ARCHIVE = 1 << 10,
        CLANG = 1 << 11,
        FORCE = 1 << 12,
        REVERT = 1 << 13,
        CUSTOM = 1 << 14,

        PREPARE = DOWNLOAD | EXTRACT | PATCH,
        MAKE = CONFIGURE | BUILD,
        FULL = PREPARE | MAKE | INSTALL
    };

    std::string kernel;
    std::string suffix;
    std::string oldKernel;
    std::string oldSuffix;
    std::string config;
    std::string patch;
    int stages = NONE;

    // Custom wins when a run names both.
    [[nodiscard]] Toolchain toolchain() const {
        if (stages & CUSTOM) {
            return Toolchain::Custom;
        }

        return stages & CLANG ? Toolchain::Llvm : Toolchain::Gcc;
    }
};

namespace WorkflowFactory {
    Workflow *create(const Options &options);
    Options autoupdate();
}


#endif //KERNELMGR_WORKFLOWFACTORY_H
