// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_COMPONENTS_PROGRESSTRACK_H
#define KERNELMGR_GUI_COMPONENTS_PROGRESSTRACK_H


#include "ttk/draw/Anim.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Widget.h"

class ProgressTrack : public ttk::Widget {
public:
    // Below zero is indeterminate.
    void set_value(int value);
    void set_tone(ttk::Theme::Tone tone);

    double natural_height(ttk::Typeface & /*type*/, double /*width*/) override { return 5.0; }

    void paint(const ttk::Painter &painter) override;
    bool advance(double now) override;

protected:
    void moved() override;
    void restyle() override { _tone.restyle(); }

private:
    int _value = -1;
    ttk::Theme::Tone _tone{&ttk::Theme::Palette::accent};
    ttk::Anim::Tween _share;
    double _started = 0.0;
    double _phase = 0.0;
};


#endif //KERNELMGR_GUI_COMPONENTS_PROGRESSTRACK_H
