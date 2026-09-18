// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <utility>

#include "gui/components/VersionList.h"
#include "gui/components/Wash.h"
#include "ttk/draw/Theme.h"
#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Root.h"

VersionList::VersionList(const bool compact, std::function<void(int)> picked)
        : _compact(compact), _picked(std::move(picked)) {
    _takesPointer = true;
}

void VersionList::set_rows(std::vector<Row> rows) {
    _rows = std::move(rows);

    if (root() != nullptr) {
        root()->relayout();
    }

    invalidate();
}

void VersionList::set_current(const int index) {
    if (index != _current) {
        _current = index;
        invalidate();
    }
}

void VersionList::arrange(ttk::Typeface & /*type*/) {
    const auto count = static_cast<double>(_rows.size());

    set_reach(count > 0.0 ? (count * row_height()) + ((count - 1.0) * row_gap()) : 0.0);
}

void VersionList::paint(const ttk::Painter &painter) {
    const ttk::Theme::Palette &palette = ttk::Theme::of();
    const double wide = _box.w - (_compact ? 2.0 : 6.0);

    painter.push(_box);

    double y = _box.y - offset();

    for (size_t index = 0; index < _rows.size(); ++index) {
        const Row &row = _rows[index];
        const BLRect line{_box.x, y, wide, row_height()};

        y += row_height() + row_gap();

        if (!painter.needed(line)) {
            continue;
        }

        const bool picked = !_compact && std::cmp_equal(index, _current);
        const bool lit = std::cmp_equal(index, _over);

        Wash::paint(painter, line, ttk::Theme::radiusSmall, picked ? 1.0 : 0.0, lit ? 1.0 : 0.0);

        if (_compact) {
            const BLFont &name = painter.font(600, ttk::Theme::fontBody);
            const double named = painter.width(name, row.version);
            double x = line.x + 12.0;

            painter.circle(BLPoint{x + 4.0, line.y + (line.h / 2.0)}, 4.0,
                           row.running ? palette.success : row.installed > 0 ? palette.accent : palette.border);
            x += 8.0 + 10.0;

            painter.label(name, BLRect{x, line.y, named + 2.0, line.h}, ttk::Align::Start, row.version,
                          palette.text);
            x += named + 10.0;

            painter.label(painter.font(400, ttk::Theme::fontSmall),
                          BLRect{x, line.y, std::max(0.0, line.x + line.w - 14.0 - x), line.h},
                          ttk::Align::Start, row.summary, palette.faint);

            continue;
        }

        if (picked) {
            painter.round(BLRect{line.x, line.y + ((line.h - 28.0) / 2.0), 3.0, 28.0}, 2.0, palette.accent);
        }

        const BLFont &name = painter.font(600, ttk::Theme::fontMedium);
        const double named = painter.width(name, row.version);
        const double tall = painter.line_height(name);
        const BLFont &small = painter.font(400, ttk::Theme::fontTiny);
        const double short_ = painter.line_height(small);
        const double top = line.y + ((line.h - (tall + 3.0 + short_)) / 2.0);
        const double left = line.x + 16.0;
        const double room = std::max(0.0, line.x + line.w - 14.0 - left);

        painter.label(name, BLRect{left, top, named + 2.0, tall}, ttk::Align::Start, row.version, palette.text);

        if (row.running) {
            painter.circle(BLPoint{left + named + 7.0 + 4.0, top + (tall / 2.0)}, 4.0, palette.success);
        }

        painter.label(small, BLRect{left, top + tall + 3.0, room, short_}, ttk::Align::Start, row.summary,
                      palette.faint);
    }

    painter.pop();
    Scroll::paint(painter);
}

int VersionList::row_at(const double y) const {
    const double step = row_height() + row_gap();
    const double at = y - _box.y + offset();

    if (at < 0.0) {
        return -1;
    }

    const auto row = static_cast<int>(at / step);

    if (std::cmp_greater_equal(row, _rows.size()) || at - (row * step) >= row_height()) {
        return -1;
    }

    return row;
}

bool VersionList::over_lane(const double x) const {
    return scrollable() && x >= _box.x + _box.w - ttk::Theme::lane;
}

ttk::Cursor VersionList::cursor_at(const double x, const double y) const {
    return !over_lane(x) && row_at(y) >= 0 ? ttk::Cursor::Pointer : ttk::Cursor::Default;
}

void VersionList::hover(const ttk::Pointer &at) {
    if (const int over = over_lane(at.x) ? -1 : row_at(at.y); over != _over) {
        _over = over;
        invalidate();
    }
}

void VersionList::leave() {
    Widget::leave();
    _over = -1;
    invalidate();
}

bool VersionList::press(const ttk::Pointer &at) {
    _scrolling = Scroll::press(at);

    return _scrolling || holds(at.x, at.y);
}

void VersionList::release(const ttk::Pointer &at) {
    Scroll::release(at);

    if (std::exchange(_scrolling, false)) {
        return;
    }

    if (const int row = row_at(at.y); row >= 0 && _picked) {
        _picked(row);
    }
}
