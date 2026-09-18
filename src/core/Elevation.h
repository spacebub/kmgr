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


#include <optional>
#include <string>

namespace Elevation {
    void remember(const std::string &secret);
    std::optional<std::string> recall();
    void forget();
    bool remembered();
}


#endif //KERNELMGR_ELEVATION_H
