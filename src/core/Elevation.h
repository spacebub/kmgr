// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_ELEVATION_H
#define KERNELMGR_ELEVATION_H


#include <functional>
#include <string>
#include <string_view>

// The one password the elevation command is given, held in memory that stays off swap and
// out of core dumps. It is never handed out as a copy: a sink is called with a view of it.
namespace Elevation {
void remember(const std::string &secret);
bool feed(const std::function<void(std::string_view secret)> &sink);
void forget();
bool remembered();

// Writes over the characters before letting them go, for the copies callers cannot avoid.
void wipe(std::string &text);
}


#endif //KERNELMGR_ELEVATION_H
