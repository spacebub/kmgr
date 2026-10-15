// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_PAGES_HOMEPAGE_H
#define KERNELMGR_GUI_PAGES_HOMEPAGE_H


#include <string_view>

#include "gui/components/EmptyState.h"
#include "gui/components/VersionList.h"
#include "ttk/toolkit/controls/Button.h"
#include "ttk/toolkit/controls/Fact.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/toolkit/controls/Pill.h"
#include "ttk/toolkit/layout/Box.h"
#include "ttk/toolkit/layout/Panel.h"

struct Reach;

class HomePage : public ttk::Box {
public:
    explicit HomePage(Reach *reach);

    void sync();

    double natural_height(ttk::Typeface &type, double width) override;
    void arrange(ttk::Typeface &type) override;

private:
    class Arrow : public ttk::Widget {
    public:
        Arrow() {
            fixedWidth = 30.0;
            fixedHeight = 15.0;
        }

        void paint(const ttk::Painter &painter) override;
    };

    class Below : public ttk::Box {
    public:
        Below() : Box(Flow::Row) {}

        using Box::set_flow;
    };

    [[nodiscard]] std::string_view status() const;

    bool stack(double width);

    Reach *_reach;
    int _catalogSeen = -1;
    std::string _shape;

    ttk::Label *_current = nullptr;
    Arrow *_arrow = nullptr;
    ttk::Label *_latest = nullptr;
    ttk::Label *_facts = nullptr;
    ttk::Pill *_status = nullptr;
    ttk::Label *_ahead = nullptr;
    ttk::Button *_retry = nullptr;
    ttk::Button *_update = nullptr;
    Below *_below = nullptr;
    ttk::Panel *_places = nullptr;
    EmptyState *_empty = nullptr;
    VersionList *_preview = nullptr;
    ttk::Fact *_base = nullptr;
    ttk::Fact *_versions = nullptr;
    ttk::Fact *_builds = nullptr;
    ttk::Fact *_archives = nullptr;
    ttk::Button *_pull = nullptr;
};


#endif //KERNELMGR_GUI_PAGES_HOMEPAGE_H
