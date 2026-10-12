// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_SHA256_H
#define KERNELMGR_SHA256_H


#include <string>
#include <string_view>

namespace Sha256 {
// Both return the digest as lowercase hex. The file one throws when the file cannot be read.
std::string text(std::string_view data);
std::string file(const std::string &path);
}


#endif //KERNELMGR_SHA256_H
