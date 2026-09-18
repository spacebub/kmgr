// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_MAKE_H
#define KERNELMGR_MAKE_H


#include <string>
#include <thread>

#include "Configuration.h"
#include "Toolchain.h"

namespace Make {
    inline int jobs() {
        if (const int configured = Configuration::get()->jobs; configured > 0) {
            return configured;
        }

        const unsigned int detected = std::thread::hardware_concurrency();

        return detected > 0 ? static_cast<int>(detected) : 1;
    }

    // The custom toolchain adds nothing of its own, so empty flags mean a plain make.
    // Characters that would end the command early are stripped.
    inline std::string flags(const Toolchain toolchain) {
        if (toolchain == Toolchain::Llvm) {
            return "LLVM=1";
        }

        if (toolchain != Toolchain::Custom) {
            return {};
        }

        std::string custom;

        for (const char character : Configuration::get()->customFlags) {
            custom += static_cast<unsigned char>(character) < ' ' ? ' ' : character;
        }

        const size_t first = custom.find_first_not_of(' ');

        return first == std::string::npos
            ? std::string{}
            : custom.substr(first, custom.find_last_not_of(' ') - first + 1);
    }

    inline std::string command(const std::string &target, const Toolchain toolchain,
                               const bool parallel = true) {
        std::string command = "make";

        if (parallel) {
            command += " -j" + std::to_string(jobs());
        }

        if (const std::string given = flags(toolchain); !given.empty()) {
            command += " " + given;
        }

        if (!target.empty()) {
            command += " " + target;
        }

        return command;
    }
}


#endif //KERNELMGR_MAKE_H
