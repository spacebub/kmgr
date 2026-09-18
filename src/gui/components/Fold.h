// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_COMPONENTS_FOLD_H
#define KERNELMGR_GUI_COMPONENTS_FOLD_H


#include "ttk/draw/Anim.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/Widget.h"
#include "ttk/toolkit/layout/Box.h"

// A card that jumps in height pulls the eye off what was being read, so the
// panel rolls out. One number drives its height, reveal and gap together.
class Fold : public ttk::Widget {
public:
    Fold() {
        _body = append(ttk::Box::column());
        _body->spacing(12.0);

        Widget::set_visible(false);
    }

    [[nodiscard]] ttk::Box *body() const { return _body; }

    [[nodiscard]] bool open() const { return _open; }

    void set_open(const bool open) {
        if (_open == open) {
            return;
        }

        _open = open;

        if (root() == nullptr) {
            _slide.set(open ? 1.0F : 0.0F);
            set_visible(open);

            return;
        }

        if (open) {
            set_visible(true);
        }

        _slide.run(open ? 1.0F : 0.0F, now(), 0.22, ttk::Anim::Curve::CubicOut);
        _settling = true;
        wake();
        root()->relayout();
    }

    double natural_height(ttk::Typeface &type, const double width) override {
        const double openness = _slide.value();

        return openness > 0.0 ? (_body->natural_height(type, width) + GAP) * openness : 0.0;
    }

    void arrange(ttk::Typeface &type) override {
        const double openness = _slide.value();
        const double tall = _body->natural_height(type, _box.w);

        _body->place(BLRect{_box.x, _box.y + (GAP * openness), _box.w, tall}, type);
    }

    bool clips(BLRect &region) const override {
        region = _box;

        return true;
    }

    void paint(const ttk::Painter &painter) override {
        const double openness = _slide.value();

        if (openness <= 0.0) {
            return;
        }

        painter.context().save();
        painter.context().set_global_alpha(openness);
        Widget::paint(painter);
        painter.context().restore();
    }

    bool advance(const double now) override {
        _slide.advance(now);

        if (_settling && root() != nullptr) {
            root()->relayout();
        }

        invalidate();

        if (!_slide.live()) {
            _settling = false;

            if (!_open) {
                set_visible(false);
            }

            return false;
        }

        return true;
    }

private:
    static constexpr double GAP = 4.0;

    ttk::Box *_body = nullptr;
    ttk::Anim::Tween _slide;
    bool _open = false;
    bool _settling = false;
};


#endif //KERNELMGR_GUI_COMPONENTS_FOLD_H
