// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_COMPONENTS_PARTS_H
#define KERNELMGR_GUI_COMPONENTS_PARTS_H


#include <functional>
#include <memory>
#include <string>

#include "ttk/draw/Glyphs.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Widget.h"
#include "ttk/toolkit/controls/GlyphButton.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/toolkit/controls/Pill.h"
#include "ttk/toolkit/layout/Box.h"

struct Reach;

namespace Parts {
    std::unique_ptr<ttk::Label> section(const std::string &text);

    std::unique_ptr<ttk::Label> text(const std::string &text, int weight, float size, BLRgba32 tone);

    std::unique_ptr<ttk::Label> note(const std::string &text);

    // Clickable, it opens `opens`, or the path itself when that is empty.
    std::unique_ptr<ttk::Label> path(Reach *reach, const std::string &path, bool clickable,
                                     const std::string &opens = {});

    std::unique_ptr<ttk::Pill> pill(const std::string &text, BLRgba32 tone, BLRgba32 wash, bool dot = true);

    std::unique_ptr<ttk::GlyphButton> glyph_button(ttk::Glyphs::Glyph glyph, double size,
                                                   const std::string &hint, BLRgba32 rest, BLRgba32 hot,
                                                   BLRgba32 wash, std::function<void()> clicked);

    std::unique_ptr<ttk::Box> above(double top, ttk::Widget::Ptr child);

    std::unique_ptr<ttk::Box> centred(ttk::Widget::Ptr child);

    class Rule : public ttk::Widget {
    public:
        explicit Rule(const double opacity = 1.0) : _opacity(opacity) { fixedHeight = 1.0; }

        void paint(const ttk::Painter &painter) override {
            painter.fill(BLRect{_box.x, _box.y, _box.w, 1.0},
                         ttk::Theme::alpha(ttk::Theme::of().border, _opacity));
        }

    private:
        double _opacity;
    };

    struct Tones {
        BLRgba32 tone;
        BLRgba32 wash;
    };

    Tones lit(bool on, BLRgba32 tone, BLRgba32 wash);

    bool shown(const ttk::Widget *widget);
}


#endif //KERNELMGR_GUI_COMPONENTS_PARTS_H
