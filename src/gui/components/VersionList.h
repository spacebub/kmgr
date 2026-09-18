// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_COMPONENTS_VERSIONLIST_H
#define KERNELMGR_GUI_COMPONENTS_VERSIONLIST_H


#include <functional>
#include <string>
#include <vector>

#include "ttk/toolkit/layout/Scroll.h"

class VersionList : public ttk::Scroll {
public:
    struct Row {
        std::string version;
        std::string summary;
        bool running = false;
        int installed = 0;
    };

    // Compact is the home page's one line per version: a dot, the version and a word.
    VersionList(bool compact, std::function<void(int)> picked);

    void set_rows(std::vector<Row> rows);
    void set_current(int index);
    [[nodiscard]] int current() const { return _current; }

    void arrange(ttk::Typeface &type) override;
    void paint(const ttk::Painter &painter) override;

    [[nodiscard]] ttk::Cursor cursor_at(double x, double y) const override;
    void hover(const ttk::Pointer &at) override;
    void leave() override;
    bool press(const ttk::Pointer &at) override;
    void release(const ttk::Pointer &at) override;

private:
    [[nodiscard]] double row_height() const { return _compact ? 42.0 : 62.0; }
    [[nodiscard]] double row_gap() const { return _compact ? 2.0 : 4.0; }
    [[nodiscard]] int row_at(double y) const;
    [[nodiscard]] bool over_lane(double x) const;

    bool _compact;
    std::function<void(int)> _picked;
    std::vector<Row> _rows;
    int _current = -1;
    int _over = -1;
    // The press went to the bar or the track, so the release is not a click.
    bool _scrolling = false;
};


#endif //KERNELMGR_GUI_COMPONENTS_VERSIONLIST_H
