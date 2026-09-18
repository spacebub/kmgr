// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_TOOLCHAIN_H
#define KERNELMGR_TOOLCHAIN_H


#include <string>

// Unknown is not a toolchain but a build directory that does not say.
enum class Toolchain {
    Unknown,
    Gcc,
    Llvm,
    Custom
};

inline std::string toolchain_name(const Toolchain toolchain) {
    switch (toolchain) {
        case Toolchain::Gcc:
            return "gcc";
        case Toolchain::Llvm:
            return "llvm";
        case Toolchain::Custom:
            return "custom";
        default:
            return {};
    }
}

// An unknown word reads back as Unknown, the same as never answered.
inline Toolchain toolchain_from(const std::string &name) {
    if (name == "gcc") {
        return Toolchain::Gcc;
    }

    if (name == "llvm") {
        return Toolchain::Llvm;
    }

    return name == "custom" ? Toolchain::Custom : Toolchain::Unknown;
}


#endif //KERNELMGR_TOOLCHAIN_H
