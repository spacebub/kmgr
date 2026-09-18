// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_PAGES_SETTINGSPAGE_H
#define KERNELMGR_GUI_PAGES_SETTINGSPAGE_H


#include <string>
#include <vector>

#include "gui/components/Choice.h"
#include "gui/components/HoldingRow.h"
#include "gui/components/LogSteps.h"
#include "gui/components/SettingRow.h"
#include "ttk/draw/Anim.h"
#include "ttk/toolkit/controls/Button.h"
#include "ttk/toolkit/controls/Field.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/toolkit/controls/Pill.h"
#include "ttk/toolkit/controls/Toggle.h"
#include "ttk/toolkit/layout/Box.h"
#include "ttk/toolkit/layout/Panel.h"
#include "ttk/toolkit/layout/Scroll.h"
#include "ttk/toolkit/Widget.h"

struct Reach;

class SettingsPage : public ttk::Widget {
public:
    explicit SettingsPage(Reach *reach);

    void sync();

    void arrange(ttk::Typeface &type) override;

private:
    class Rail : public ttk::Widget {
    public:
        explicit Rail(SettingsPage *page);

        void paint(const ttk::Painter &painter) override;
        [[nodiscard]] ttk::Cursor cursor_at(double x, double y) const override;
        void hover(const ttk::Pointer &at) override;
        void leave() override;
        bool press(const ttk::Pointer &at) override;
        void release(const ttk::Pointer &at) override;

    private:
        static constexpr double ROW = 34.0;
        static constexpr double GAP = 2.0;

        [[nodiscard]] int row_at(double y) const;

        SettingsPage *_page;
        int _over = -1;
    };

    // The whole card is the control, since the switch alone is a small target.
    class SwitchCard : public ttk::Panel {
    public:
        explicit SwitchCard(std::function<void()> flipped);

        bool press(const ttk::Pointer & /*at*/) override { return true; }
        void release(const ttk::Pointer &at) override;
        void enter() override;
        void leave() override;

    private:
        std::function<void()> _flipped;
    };

    class ToolchainRow : public ttk::Widget {
    public:
        ToolchainRow(ttk::Widget::Ptr texts, ttk::Widget::Ptr choice);

        double natural_height(ttk::Typeface &type, double width) override;
        void arrange(ttk::Typeface &type) override;
        void paint(const ttk::Painter &painter) override;

    private:
        ttk::Widget *_texts = nullptr;
        ttk::Widget *_choice = nullptr;
    };

    // In switch order, spelled the way the settings file writes them.
    static constexpr const char *TOOLCHAINS[] = { "gcc", "llvm", "custom" };
    static constexpr const char *SECTIONS[] = { "General", "Build", "Password", "Logs" };

    [[nodiscard]] int wanted_jobs() const;
    [[nodiscard]] bool dirty() const;

    void load();
    void store();
    void read_logs();
    void sync_holdings();

    Reach *_reach;

    int _section = 0;
    std::string _wantedCompiler = "gcc";
    bool _wantedNotifications = false;
    std::string _logSize;
    std::vector<Log::Holding> _holdings;
    std::string _openedLog;
    int _logsRead = -1;
    int _settingsSeen = -1;
    std::vector<std::string> _toolchainNames;

    Rail *_rail = nullptr;
    std::vector<ttk::Scroll *> _panes;
    ttk::Panel *_footer = nullptr;

    SettingRow *_base = nullptr;
    SettingRow *_archives = nullptr;
    SettingRow *_boot = nullptr;
    SettingRow *_grub = nullptr;
    SettingRow *_cdn = nullptr;
    SettingRow *_format = nullptr;
    ttk::Toggle *_notifications = nullptr;
    Choice *_toolchain = nullptr;
    SettingRow *_jobs = nullptr;
    ttk::Field *_flags = nullptr;
    SettingRow *_elevation = nullptr;
    ttk::Button *_forget = nullptr;
    ttk::Pill *_logsSize = nullptr;
    ttk::Label *_logsNote = nullptr;
    ttk::Button *_deleteAll = nullptr;
    ttk::Widget *_logsRule = nullptr;
    ttk::Label *_noLogs = nullptr;
    ttk::Box *_holdingsList = nullptr;
    std::vector<HoldingRow *> _rows;
    std::vector<ttk::Box *> _stepsBoxes;
    std::vector<LogSteps *> _steps;
};


#endif //KERNELMGR_GUI_PAGES_SETTINGSPAGE_H
