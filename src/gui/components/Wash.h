// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_COMPONENTS_WASH_H
#define KERNELMGR_GUI_COMPONENTS_WASH_H


#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Painter.h"

// Two layers rather than one blended colour: the washes differ widely in alpha,
// and a blend runs the alpha up before the colour moves, which reads as a flash.
namespace Wash {
    inline void paint(const ttk::Painter &painter, const BLRect &box, const double rounding,
                      const double selected, const double hovered) {
        const ttk::Theme::Palette &palette = ttk::Theme::of();

        if (selected > 0.0) {
            painter.round(box, rounding, ttk::Theme::alpha(palette.accentSoft, selected));
        }

        if (hovered > 0.0 && selected <= 0.0) {
            painter.round(box, rounding, ttk::Theme::alpha(palette.hover, hovered));
        }
    }
}


#endif //KERNELMGR_GUI_COMPONENTS_WASH_H
