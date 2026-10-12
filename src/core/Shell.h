// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_SHELL_H
#define KERNELMGR_SHELL_H


#include <string>

namespace Shell {
// Single quotes, so nothing inside is expanded or ends the word early.
inline std::string quote(const std::string &word) {
    std::string quoted = "'";

    for (const char character : word) {
        if (character == '\'') {
            quoted += "'\\''";
        } else {
            quoted += character;
        }
    }

    quoted += '\'';

    return quoted;
}
}


#endif //KERNELMGR_SHELL_H
