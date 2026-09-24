// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <utility>

#include "gui/components/SectionTabs.h"
#include "ttk/draw/Theme.h"
#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Root.h"

namespace {
    constexpr double SIDES = 26.0;
    constexpr double GAP = 2.0;
    constexpr double NOTE_GAP = 7.0;
    constexpr double NOTE_PAD = 12.0;
    constexpr double NOTE_HEIGHT = 18.0;
}

SectionTabs::SectionTabs(std::function<void(int)> selected) : _selected(std::move(selected)) {
    _takesPointer = true;
    cursor = ttk::Cursor::Pointer;
}

void SectionTabs::set_tabs(std::vector<Tab> tabs) {
    _tabs.clear();

    for (Tab &tab : tabs) {
        _tabs.push_back(Held{.tab = std::move(tab)});
    }

    if (root() != nullptr) {
        root()->relayout();
    }
}

void SectionTabs::set_note(const int index, const std::string &note) {
    if (index < 0 || std::cmp_greater_equal(index, _tabs.size())
        || _tabs[static_cast<size_t>(index)].tab.note == note) {
        return;
    }

    _tabs[static_cast<size_t>(index)].tab.note = note;

    if (root() != nullptr) {
        root()->relayout();
    }
}

void SectionTabs::set_current(const int index) {
    if (index == _current) {
        return;
    }

    _current = index;

    if (root() != nullptr) {
        root()->relayout();
    }
}

void SectionTabs::arrange(ttk::Typeface &type) {
    double at = _box.x;

    for (size_t index = 0; index < _tabs.size(); ++index) {
        Held &held = _tabs[index];

        held.label = std::max(type.width(type.at(400, ttk::Theme::fontSmall), held.tab.label),
                              type.width(type.at(600, ttk::Theme::fontSmall), held.tab.label));
        held.note = held.tab.note.empty()
            ? 0.0
            : type.width(type.at(600, ttk::Theme::fontTiny), held.tab.note) + NOTE_PAD;

        const double content = held.label + (held.note > 0.0 ? NOTE_GAP + held.note : 0.0);

        held.box = BLRect{at, _box.y, content + SIDES, _box.h};
        at += held.box.w + GAP;
    }
}

void SectionTabs::paint(const ttk::Painter &painter) {
    const ttk::Theme::Palette &palette = ttk::Theme::palette();

    // Laid down first so the mark under the picked tab covers it.
    painter.fill(BLRect{_box.x, _box.y + _box.h - 1.0, _box.w, 1.0}, palette.border);

    for (size_t index = 0; index < _tabs.size(); ++index) {
        const Held &held = _tabs[index];
        const bool active = std::cmp_equal(index, _current);
        const bool lit = std::cmp_equal(index, _over);

        if (active || lit) {
            painter.fill(BLRect{held.box.x, held.box.y + held.box.h - 1.0, held.box.w, 2.0},
                         active ? palette.accent : palette.borderStrong);
        }

        const double content = held.label + (held.note > 0.0 ? NOTE_GAP + held.note : 0.0);
        const double x = held.box.x + ((held.box.w - content) / 2.0);

        painter.label(painter.font(active ? 600 : 400, ttk::Theme::fontSmall),
                      BLRect{x, held.box.y, held.label, held.box.h}, ttk::Align::Centre, held.tab.label,
                      active ? palette.accent : lit ? palette.text : palette.muted);

        if (held.note > 0.0) {
            const BLRect note{x + held.label + NOTE_GAP, held.box.y + ((held.box.h - NOTE_HEIGHT) / 2.0),
                              held.note, NOTE_HEIGHT};

            painter.round(note, NOTE_HEIGHT / 2.0, active ? palette.accentSoft : palette.mutedSoft);
            painter.label(painter.font(600, ttk::Theme::fontTiny), note, ttk::Align::Centre, held.tab.note,
                          active ? palette.accent : palette.faint);
        }
    }
}

int SectionTabs::at_point(const double x, const double y) const {
    for (size_t index = 0; index < _tabs.size(); ++index) {
        if (const BLRect &box = _tabs[index].box;
            x >= box.x && x < box.x + box.w && y >= box.y && y < box.y + box.h) {
            return static_cast<int>(index);
        }
    }

    return -1;
}

bool SectionTabs::press(const ttk::Pointer &at) {
    return at_point(at.x, at.y) >= 0;
}

void SectionTabs::release(const ttk::Pointer &at) {
    if (const int index = at_point(at.x, at.y); index >= 0 && _selected) {
        _selected(index);
    }
}

void SectionTabs::hover(const ttk::Pointer &at) {
    if (const int over = at_point(at.x, at.y); over != _over) {
        _over = over;
        invalidate();
    }
}

void SectionTabs::leave() {
    Widget::leave();
    _over = -1;
    invalidate();
}
