// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_COMPONENTS_SECTIONTABS_H
#define KERNELMGR_GUI_COMPONENTS_SECTIONTABS_H


#include <functional>
#include <string>
#include <vector>

#include "ttk/toolkit/Widget.h"

class SectionTabs : public ttk::Widget {
public:
    struct Tab {
        std::string label;
        std::string note;
    };

    explicit SectionTabs(std::function<void(int)> selected);

    void set_tabs(std::vector<Tab> tabs);
    void set_note(int index, const std::string &note);
    void set_current(int index);
    [[nodiscard]] int current() const { return _current; }

    double natural_height(ttk::Typeface & /*type*/, double /*width*/) override { return 35.0; }

    void arrange(ttk::Typeface &type) override;
    void paint(const ttk::Painter &painter) override;

    bool press(const ttk::Pointer &at) override;
    void release(const ttk::Pointer &at) override;
    void hover(const ttk::Pointer &at) override;
    void leave() override;

private:
    struct Held {
        Tab tab;
        BLRect box{};
        double label = 0.0;
        double note = 0.0;
    };

    [[nodiscard]] int at_point(double x, double y) const;

    std::function<void(int)> _selected;
    std::vector<Held> _tabs;
    int _current = 0;
    int _over = -1;
};


#endif //KERNELMGR_GUI_COMPONENTS_SECTIONTABS_H
