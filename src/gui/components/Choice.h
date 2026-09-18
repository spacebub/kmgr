// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_COMPONENTS_CHOICE_H
#define KERNELMGR_GUI_COMPONENTS_CHOICE_H


#include <functional>
#include <string>
#include <vector>

#include "ttk/draw/Anim.h"
#include "ttk/toolkit/Widget.h"

class Choice : public ttk::Widget {
public:
    explicit Choice(std::function<void(int)> selected);

    void set_options(std::vector<std::string> options);
    void set_current(int index);
    [[nodiscard]] int current() const { return _current; }

    void set_disabled(std::vector<int> disabled);
    void set_disabled_hint(std::string text);
    void set_hint(std::string text);

    double natural_width(ttk::Typeface &type) override;
    double natural_height(ttk::Typeface & /*type*/, double /*width*/) override { return 30.0; }

    void arrange(ttk::Typeface &type) override;
    void paint(const ttk::Painter &painter) override;

    bool press(const ttk::Pointer &at) override;
    void release(const ttk::Pointer &at) override;
    void hover(const ttk::Pointer &at) override;
    void leave() override;
    [[nodiscard]] ttk::Cursor cursor_at(double x, double y) const override;

    bool advance(double now) override;

private:
    [[nodiscard]] bool blocked(int index) const;
    [[nodiscard]] int at_point(double x) const;
    void measure(ttk::Typeface &type);

    struct Word {
        std::string text;
        BLRect box{};
    };

    std::function<void(int)> _selected;
    std::vector<Word> _words;
    std::vector<int> _disabled;
    std::string _hint;
    std::string _disabledHint;
    int _current = 0;
    int _under = -1;

    ttk::Anim::Tween _markX;
    ttk::Anim::Tween _markWidth;
    bool _marked = false;
};


#endif //KERNELMGR_GUI_COMPONENTS_CHOICE_H
