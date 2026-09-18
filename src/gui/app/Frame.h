// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_APP_FRAME_H
#define KERNELMGR_GUI_APP_FRAME_H


#include "gui/app/TitleBar.h"
#include "ttk/toolkit/Widget.h"

class Frame : public ttk::Widget {
public:
    Frame(TitleBar *bar, Widget *pages) : _bar(bar), _pages(pages) {}

    void arrange(ttk::Typeface &type) override;
    void paint(const ttk::Painter &painter) override;

private:
    TitleBar *_bar;
    Widget *_pages;
};


#endif //KERNELMGR_GUI_APP_FRAME_H
