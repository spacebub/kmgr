// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>

#include "gui/app/Frame.h"
#include "gui/app/Look.h"
#include "ttk/draw/Theme.h"

namespace {
    constexpr double MARGIN = 20.0;
}

void Frame::arrange(ttk::Typeface &type) {
    _bar->place(BLRect{_box.x, _box.y, _box.w, Look::barHeight}, type);

    const double room = std::max(0.0, _box.w - (MARGIN * 2.0));
    const double page = std::min(room, Look::pageWidth);
    const double top = _box.y + Look::barHeight + MARGIN;
    const double tall = std::max(0.0, _box.y + _box.h - top - MARGIN);

    _pages->place(BLRect{_box.x + ((_box.w - page) / 2.0), top, page, tall}, type);
}

void Frame::paint(const ttk::Painter &painter) {
    painter.fill(_box, ttk::Theme::of().background);
    Widget::paint(painter);
}
