// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_PROGRAM_H
#define KERNELMGR_PROGRAM_H


#include <string>

namespace Program {
// The release number, followed by the git revision unless this is a release build.
std::string version();
}


#endif //KERNELMGR_PROGRAM_H
