// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_APP_APP_H
#define KERNELMGR_GUI_APP_APP_H


#include <string>

#include "gui/app/Frame.h"
#include "gui/app/Reach.h"
#include "gui/app/TitleBar.h"
#include "gui/components/Buzzes.h"
#include "gui/model/Catalog.h"
#include "gui/model/Logs.h"
#include "gui/model/SettingsBridge.h"
#include "gui/model/SystemStatus.h"
#include "gui/model/WorkflowRunner.h"
#include "gui/overlays/WorkflowSheet.h"
#include "gui/pages/HomePage.h"
#include "gui/pages/KernelsPage.h"
#include "gui/pages/SettingsPage.h"
#include "gui/services/DesktopAlert.h"
#include "ttk/dialogs/DialogLayer.h"
#include "ttk/dialogs/FilePicker.h"
#include "ttk/notices/Notifier.h"
#include "ttk/shell/Shell.h"
#include "ttk/toolkit/overlays/Dialog.h"
#include "ttk/toolkit/overlays/Tips.h"

class App {
public:
    // The window is already open: everything here posts to its loop.
    explicit App(ttk::Shell &shell);

    App(const App &) = delete;
    App &operator=(const App &) = delete;
    App(App &&) = delete;
    App &operator=(App &&) = delete;

    void run();

private:
    void build();
    void wire();
    void sync();
    void touch();
    void go(Page page);
    void show(const std::string &version);
    void cycle_shade();
    void ask(const std::string &title, const std::string &body, const std::string &accept, bool danger,
             std::function<void()> accepted);
    void pick(const std::string &title, const std::vector<std::string> &filters, bool directories,
              std::function<void(const std::string &)> chosen);
    bool shortcut(const ttk::Key &pressed);
    void ask_about_notifications();

    ttk::Shell &_shell;
    ttk::Notifier _notifier;
    DesktopAlert _alert;
    Catalog _catalog;
    SystemStatus _system;
    WorkflowRunner _workflow;
    SettingsBridge _settings;
    Logs _logs;
    ttk::FilePicker _picker;

    // Declared last of the services, so every reference in it is already built.
    Reach _reach;

    TitleBar *_bar = nullptr;
    ttk::Widget *_pages = nullptr;
    HomePage *_home = nullptr;
    KernelsPage *_kernels = nullptr;
    SettingsPage *_settingsPage = nullptr;
    WorkflowSheet *_sheet = nullptr;
    ttk::DialogLayer *_dialogs = nullptr;
    ttk::Dialog *_pick = nullptr;
    Buzzes *_buzzes = nullptr;
    ttk::Tips *_tips = nullptr;

    Page _page = Page::Home;
    bool _dirty = true;
};


#endif //KERNELMGR_GUI_APP_APP_H
