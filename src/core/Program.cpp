// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "Program.h"
#include "kernelmgr_git_revision.h"

std::string Program::version() {
    std::string version = KERNELMGR_VERSION;

    if constexpr (*KERNELMGR_GIT_REVISION != '\0') {
        version += " (" KERNELMGR_GIT_REVISION ")";
    }

    return version;
}
