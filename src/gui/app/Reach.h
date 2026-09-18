// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_APP_REACH_H
#define KERNELMGR_GUI_APP_REACH_H


#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "gui/model/Catalog.h"
#include "gui/model/Logs.h"
#include "gui/model/SettingsBridge.h"
#include "gui/model/SystemStatus.h"
#include "gui/model/WorkflowRunner.h"
#include "ttk/notices/Notifier.h"
#include "ttk/util/Window.h"

enum class Page : std::uint8_t {
    Home,
    Kernels,
    Settings
};

// A view holding this needs nothing back from the window, so neither has to
// know the other's type.
struct Reach {
    ttk::Window &window;
    ttk::Notifier &notify;
    Catalog &catalog;
    SystemStatus &system;
    SettingsBridge &settings;
    WorkflowRunner &workflow;
    Logs &logs;

    std::function<void()> touch;

    std::function<void(Page page)> go;

    // Empty keeps the version that was showing.
    std::function<void(const std::string &version)> show;

    std::function<void()> cycleShade;

    std::function<void()> refresh;

    std::function<void(const std::string &title, const std::string &body,
                       const std::string &accept, bool danger,
                       std::function<void()> accepted)> ask;

    std::function<void(const std::string &title, const std::vector<std::string> &filters,
                       bool directories, std::function<void(const std::string &)> chosen)> pick;
};


#endif //KERNELMGR_GUI_APP_REACH_H
