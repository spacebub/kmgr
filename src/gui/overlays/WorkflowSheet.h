// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_OVERLAYS_WORKFLOWSHEET_H
#define KERNELMGR_GUI_OVERLAYS_WORKFLOWSHEET_H


#include <string>
#include <vector>

#include "gui/components/ProgressTrack.h"
#include "ttk/toolkit/controls/Button.h"
#include "ttk/toolkit/controls/Field.h"
#include "ttk/toolkit/controls/GlyphButton.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/toolkit/controls/Pill.h"
#include "ttk/toolkit/controls/TextView.h"
#include "ttk/toolkit/layout/Box.h"
#include "ttk/toolkit/layout/Panel.h"
#include "ttk/toolkit/layout/Scroll.h"
#include "ttk/toolkit/Widget.h"

struct Reach;

class WorkflowSheet : public ttk::Widget {
public:
    explicit WorkflowSheet(Reach *reach);

    void sync();
    void write(const std::string &text);

    [[nodiscard]] bool minimized() const { return _minimized; }
    void set_minimized(bool value);

    // Escape: a finished run is put away, a running one is put aside.
    void escape();

    void arrange(ttk::Typeface &type) override;
    void paint(const ttk::Painter &painter) override;
    [[nodiscard]] BLRect drawn() const override;
    Widget *at(double x, double y) override;
    bool press(const ttk::Pointer & /*at*/) override { return true; }

private:
    class Console : public ttk::Widget {
    public:
        Console();

        void write(const std::string &text);
        void clear();

        void arrange(ttk::Typeface &type) override;
        void paint(const ttk::Painter &painter) override;

    private:
        static constexpr size_t LIMIT = 65000;

        ttk::Scroll *_scroll = nullptr;
        ttk::TextView *_view = nullptr;
        std::vector<std::string> _lines;
        std::string _tail;
        size_t _held = 0;
        // Output landed while the view was at the end, so it goes there again once
        // the new height is known.
        bool _follow = false;
    };

    class Strip : public ttk::Widget {
    public:
        explicit Strip(WorkflowSheet *sheet);

        void sync(BLRgba32 edge);

        void paint(const ttk::Painter &painter) override;
        [[nodiscard]] BLRect drawn() const override;
        bool press(const ttk::Pointer & /*at*/) override { return true; }
        void release(const ttk::Pointer &at) override;
        void enter() override;
        void leave() override;

        ttk::Box *content = nullptr;

    private:
        WorkflowSheet *_sheet;
        BLRgba32 _edge;
        bool _edgeDark;
    };

    class Throbber : public ttk::Widget {
    public:
        Throbber() {
            fixedWidth = (5.0 * 3.0) + (3.0 * 2.0);
            fixedHeight = 5.0;
        }

        void paint(const ttk::Painter &painter) override;
        bool advance(double now) override;

        BLRgba32 tone{};
        bool toneDark = true;

        // Only while the strip is up and the run is going.
        bool live = false;

    private:
        double _started = 0.0;
        double _now = 0.0;
    };

    void send();

    Reach *_reach;
    bool _minimized = false;
    bool _wasRunning = false;
    bool _hadPrompt = false;
    std::string _shape;

    ttk::Panel *_panel = nullptr;
    ttk::Label *_title = nullptr;
    ttk::Label *_task = nullptr;
    ttk::Label *_detail = nullptr;
    ttk::Pill *_running = nullptr;
    ttk::Pill *_finished = nullptr;
    ProgressTrack *_track = nullptr;
    Console *_console = nullptr;
    ttk::Panel *_asking = nullptr;
    ttk::Label *_prompt = nullptr;
    ttk::Field *_answer = nullptr;
    ttk::Button *_send = nullptr;
    ttk::Label *_logPath = nullptr;
    ttk::Button *_cancel = nullptr;
    ttk::Button *_close = nullptr;

    Strip *_strip = nullptr;
    Throbber *_throbber = nullptr;
    ttk::Label *_stripTitle = nullptr;
    ttk::GlyphButton *_stripClose = nullptr;
    ProgressTrack *_stripTrack = nullptr;
    ttk::Label *_stripTask = nullptr;
    ttk::Label *_stripDetail = nullptr;
};


#endif //KERNELMGR_GUI_OVERLAYS_WORKFLOWSHEET_H
