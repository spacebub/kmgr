// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_COMPONENTS_EMPTYSTATE_H
#define KERNELMGR_GUI_COMPONENTS_EMPTYSTATE_H


#include <string>

#include "ttk/draw/Theme.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/toolkit/layout/Box.h"

class EmptyState : public ttk::Box {
public:
    EmptyState(const std::string &title, const std::string &body) : Box(Flow::Column) {
        spacing(6.0);

        _title = append(std::make_unique<ttk::Label>(title));
        _title->font(600, ttk::Theme::fontMedium)->tone(ttk::Theme::of().muted);

        _body = append(std::make_unique<ttk::Label>(body));
        _body->font(400, ttk::Theme::fontSmall)->tone(ttk::Theme::of().faint)->wrap();
    }

    void set_body(const std::string &body) const {
        _body->set_text(body);
    }

private:
    ttk::Label *_title = nullptr;
    ttk::Label *_body = nullptr;
};


#endif //KERNELMGR_GUI_COMPONENTS_EMPTYSTATE_H
