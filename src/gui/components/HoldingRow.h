// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_COMPONENTS_HOLDINGROW_H
#define KERNELMGR_GUI_COMPONENTS_HOLDINGROW_H


#include <functional>
#include <string>

#include "core/Log.h"
#include "ttk/toolkit/controls/GlyphButton.h"
#include "ttk/toolkit/Widget.h"

struct Reach;

class HoldingRow : public ttk::Widget {
public:
    HoldingRow(Reach *reach, Log::Holding holding, std::function<void()> toggled);

    void set_open(bool open);
    void set_idle(bool idle) const;

    [[nodiscard]] const std::string &name() const { return _holding.name; }

    double natural_height(ttk::Typeface & /*type*/, double /*width*/) override { return 34.0; }

    void arrange(ttk::Typeface &type) override;
    void paint(const ttk::Painter &painter) override;

    bool press(const ttk::Pointer &at) override;
    void release(const ttk::Pointer &at) override;
    void enter() override;
    void leave() override;

private:
    Reach *_reach;
    Log::Holding _holding;
    std::function<void()> _toggled;
    ttk::GlyphButton *_folder = nullptr;
    ttk::GlyphButton *_bin = nullptr;
    bool _open = false;
};


#endif //KERNELMGR_GUI_COMPONENTS_HOLDINGROW_H
