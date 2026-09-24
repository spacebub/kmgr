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

#include "gui/components/Choice.h"
#include "ttk/draw/Theme.h"
#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Root.h"

namespace {
    constexpr double SIDES = 3.0;
    constexpr double GAP = 2.0;
    constexpr double LEAST = 56.0;
}

Choice::Choice(std::function<void(int)> selected) : _selected(std::move(selected)) {
    _takesPointer = true;
}

void Choice::set_options(std::vector<std::string> options) {
    _words.clear();

    for (std::string &option : options) {
        _words.push_back(Word{.text = std::move(option)});
    }

    _marked = false;

    if (root() != nullptr) {
        root()->relayout();
    }
}

void Choice::set_current(const int index) {
    if (index == _current) {
        return;
    }

    _current = index;

    if (root() != nullptr) {
        root()->relayout();
    }

    invalidate();
}

void Choice::set_disabled(std::vector<int> disabled) {
    if (_disabled != disabled) {
        _disabled = std::move(disabled);
        invalidate();
    }
}

void Choice::set_disabled_hint(std::string text) {
    _disabledHint = std::move(text);
}

void Choice::set_hint(std::string text) {
    _hint = std::move(text);
    hint = _hint;
}

bool Choice::blocked(const int index) const {
    return std::ranges::find(_disabled, index) != _disabled.end();
}

void Choice::measure(ttk::Typeface &type) {
    for (size_t index = 0; index < _words.size(); ++index) {
        Word &word = _words[index];
        const BLFont &face = type.at(std::cmp_equal(index, _current) ? 600 : 400, ttk::Theme::fontSmall);

        word.box.w = std::max(LEAST, type.width(face, word.text) + 20.0);
    }
}

double Choice::natural_width(ttk::Typeface &type) {
    measure(type);

    double total = 0.0;

    for (const Word &word : _words) {
        total += word.box.w + GAP;
    }

    return std::max(0.0, total - GAP) + (SIDES * 2.0);
}

// Placed over its word rather than stepped by a width, since the words differ in length.
void Choice::arrange(ttk::Typeface &type) {
    measure(type);

    double total = 0.0;

    for (const Word &word : _words) {
        total += word.box.w + GAP;
    }

    total = std::max(0.0, total - GAP);

    double at = _box.x + ((_box.w - total) / 2.0);

    for (Word &word : _words) {
        word.box = BLRect{at, _box.y + SIDES, word.box.w, _box.h - (SIDES * 2.0)};
        at += word.box.w + GAP;
    }

    if (_current < 0 || std::cmp_greater_equal(_current, _words.size())) {
        return;
    }

    const BLRect &picked = _words[static_cast<size_t>(_current)].box;

    if (!_marked || root() == nullptr) {
        _markX.set(static_cast<float>(picked.x));
        _markWidth.set(static_cast<float>(picked.w));
        _marked = true;

        return;
    }

    _markX.toward(static_cast<float>(picked.x), now(), 0.15, ttk::Anim::Curve::CubicOut);
    _markWidth.toward(static_cast<float>(picked.w), now(), 0.15, ttk::Anim::Curve::CubicOut);
    wake();
}

void Choice::paint(const ttk::Painter &painter) {
    const ttk::Theme::Palette &palette = ttk::Theme::palette();

    painter.round(_box, ttk::Theme::radiusSmall, palette.sunken);
    painter.outline(_box, ttk::Theme::radiusSmall, 1.0, palette.border);

    if (_marked) {
        const BLRect mark{_markX.value(), _box.y + SIDES, _markWidth.value(), _box.h - (SIDES * 2.0)};

        painter.round(mark, ttk::Theme::radiusSmall - 2.0, palette.surface);
        painter.outline(mark, ttk::Theme::radiusSmall - 2.0, 1.0, palette.borderStrong);
    }

    for (size_t index = 0; index < _words.size(); ++index) {
        const Word &word = _words[index];
        const bool picked = std::cmp_equal(index, _current);
        const BLRgba32 ink = picked ? palette.text : palette.muted;

        painter.label(painter.font(picked ? 600 : 400, ttk::Theme::fontSmall), word.box, ttk::Align::Centre,
                      word.text, blocked(static_cast<int>(index)) ? ttk::Theme::alpha(ink, 0.45) : ink);
    }
}

int Choice::at_point(const double x) const {
    for (size_t index = 0; index < _words.size(); ++index) {
        if (const BLRect &box = _words[index].box; x >= box.x && x < box.x + box.w) {
            return static_cast<int>(index);
        }
    }

    return -1;
}

bool Choice::press(const ttk::Pointer &at) {
    const int index = at_point(at.x);

    return index >= 0 && !blocked(index);
}

void Choice::release(const ttk::Pointer &at) {
    if (const int index = at_point(at.x); index >= 0 && !blocked(index) && _selected) {
        _selected(index);
    }
}

// Set on the control, since its one hint is the one that shows.
void Choice::hover(const ttk::Pointer &at) {
    _under = at_point(at.x);
    hint = _under >= 0 && blocked(_under) && !_disabledHint.empty() ? _disabledHint : _hint;
}

void Choice::leave() {
    Widget::leave();
    _under = -1;
    hint = _hint;
}

ttk::Cursor Choice::cursor_at(const double x, double /*y*/) const {
    const int index = at_point(x);

    return index >= 0 && !blocked(index) ? ttk::Cursor::Pointer : ttk::Cursor::Default;
}

bool Choice::advance(const double now) {
    _markX.advance(now);
    _markWidth.advance(now);
    invalidate();

    return _markX.live() || _markWidth.live();
}
