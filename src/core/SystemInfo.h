// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2024
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_SYSTEMINFO_H
#define KERNELMGR_SYSTEMINFO_H


#include <string>
#include <fstream>
#include <sstream>

#include "Command.h"
#include "Toolchain.h"

enum SbctlStatus {
    Missing = 0,
    Present = 1
};

struct SystemInfo {
    // What the running kernel was made with, read off the banner it left behind.
    Toolchain compiler;
    SbctlStatus sbctlStatus;

    static SystemInfo get() {
        const bool sbctl = Command::exists("sbctl");
        std::stringstream stream;

        if (std::ifstream file("/proc/version"); !file.is_open()) {
            stream << "";
        } else {
            stream << file.rdbuf();
        }

        return {
            .compiler = stream.str().find("clang") != std::string::npos ? Toolchain::Llvm : Toolchain::Gcc,
            .sbctlStatus = sbctl ? Present : Missing
        };
    }
};


#endif //KERNELMGR_SYSTEMINFO_H
