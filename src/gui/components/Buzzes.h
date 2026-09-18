// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_COMPONENTS_BUZZES_H
#define KERNELMGR_GUI_COMPONENTS_BUZZES_H


#include "gui/app/Look.h"
#include "ttk/notices/Toasts.h"

class Buzzes : public ttk::Toasts {
public:
    using Toasts::Toasts;

    void arrange(ttk::Typeface &type) override {
        const double right = _box.x + _box.w - 18.0;
        double y = _box.y + Look::barHeight + 18.0;

        for (const Ptr &child : children()) {
            const double tall = child->natural_height(type, WIDTH);

            child->place(BLRect{right - WIDTH, y, WIDTH, tall}, type);
            y += tall + 10.0;
        }
    }

private:
    static constexpr double WIDTH = 392.0;
};


#endif //KERNELMGR_GUI_COMPONENTS_BUZZES_H
