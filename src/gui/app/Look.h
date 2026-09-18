// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_APP_LOOK_H
#define KERNELMGR_GUI_APP_LOOK_H


#include <string>

#include "ttk/notices/Notifier.h"

namespace Look {
    // A page stops at this width and centres, so a wide window does not put a
    // foot of nothing between a label and its control.
    constexpr double pageWidth = 1080.0;

    constexpr double barHeight = 56.0;

    // Call before anything is built.
    void install();

    std::string mode_name();

    void cycle(ttk::Notifier *notifier);
}


#endif //KERNELMGR_GUI_APP_LOOK_H
