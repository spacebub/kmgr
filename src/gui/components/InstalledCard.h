// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_COMPONENTS_INSTALLEDCARD_H
#define KERNELMGR_GUI_COMPONENTS_INSTALLEDCARD_H


#include <functional>
#include <string>

#include "gui/model/Catalog.h"
#include "ttk/toolkit/controls/Button.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/toolkit/controls/Pill.h"
#include "ttk/toolkit/layout/Panel.h"

class InstalledCard : public ttk::Panel {
public:
    InstalledCard(const Catalog::Build &kernel, std::function<void(const std::string &)> action);

    void sync(const Catalog::Build &kernel, bool idle, bool signable, bool archived, const std::string &version);

    [[nodiscard]] const std::string &name() const { return _name; }

private:
    std::function<void(const std::string &)> _action;
    std::string _name;

    ttk::Label *_title = nullptr;
    ttk::Pill *_running = nullptr;
    ttk::Pill *_modules = nullptr;
    ttk::Pill *_image = nullptr;
    ttk::Pill *_initramfs = nullptr;
    ttk::Pill *_signed = nullptr;
    ttk::Button *_download = nullptr;
    ttk::Button *_sign = nullptr;
    ttk::Button *_remove = nullptr;
};


#endif //KERNELMGR_GUI_COMPONENTS_INSTALLEDCARD_H
