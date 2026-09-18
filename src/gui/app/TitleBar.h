// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_APP_TITLEBAR_H
#define KERNELMGR_GUI_APP_TITLEBAR_H


#include <string>
#include <vector>

#include <blend2d/blend2d.h>

#include "gui/app/Reach.h"
#include "ttk/draw/Anim.h"
#include "ttk/toolkit/controls/GlyphButton.h"
#include "ttk/toolkit/Widget.h"

class TitleBar : public ttk::Widget {
public:
    TitleBar(Reach *reach, BLImage mark);

    void sync(Page page, bool homeBadge, const std::string &trailing);

    // True where the window manager may take the press: anywhere but a control.
    [[nodiscard]] bool draggable(double x, double y) const;

    void arrange(ttk::Typeface &type) override;
    void paint(const ttk::Painter &painter) override;

    bool press(const ttk::Pointer &at) override;
    void release(const ttk::Pointer &at) override;
    void hover(const ttk::Pointer &at) override;
    void leave() override;
    bool advance(double now) override;

private:
    struct Tab {
        Tab(const Page key, std::string label) : key(key), label(std::move(label)) {}

        Page key;
        std::string label;
        bool badge = false;
        BLRect box{};
        double width = 0.0;
        ttk::Anim::Tween lit;
        ttk::Anim::Tween on;
    };

    [[nodiscard]] bool compact() const { return _box.w < 660.0; }
    [[nodiscard]] int tab_at(double x, double y) const;

    Reach *_reach;
    BLImage _mark;
    std::vector<Tab> _tabs;
    std::string _trailing;

    ttk::GlyphButton *_shade = nullptr;
    ttk::GlyphButton *_minimize = nullptr;
    ttk::GlyphButton *_maximize = nullptr;
    ttk::GlyphButton *_close = nullptr;

    double _brandEnd = 0.0;
    int _over = -1;
};


#endif //KERNELMGR_GUI_APP_TITLEBAR_H
