// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>

#include "gui/components/SettingRow.h"
#include "ttk/draw/Theme.h"
#include "ttk/draw/Typeface.h"

SettingRow::SettingRow(Reach *reach,
                       const std::string &label,
                       const std::string &description,
                       std::function<void(const std::string &)> edited) {
    _label = append(std::make_unique<ttk::Label>(label));
    _label->font(600, ttk::Theme::fontBody)->tone(&ttk::Theme::Palette::text);

    _description = append(std::make_unique<ttk::Label>(description));
    _description->font(400, ttk::Theme::fontSmall)->tone(&ttk::Theme::Palette::faint);
    _description->set_visible(!description.empty());

    _field = append(std::make_unique<PathField>(reach, "", std::move(edited)));
}

SettingRow *SettingRow::browse(const std::string &title) {
    _field->browse(title, {"*"}, true);

    return this;
}

SettingRow *SettingRow::placeholder(const std::string &text) {
    _field->placeholder(text);

    return this;
}

SettingRow *SettingRow::mono(const bool value) {
    _field->mono(value);

    return this;
}

SettingRow *SettingRow::last(const bool value) {
    _last = value;

    return this;
}

double SettingRow::texts_width(ttk::Typeface &type) const {
    const double name = _label->natural_width(type);

    return _description->visible() ? std::max(name, _description->natural_width(type)) : name;
}

double SettingRow::texts_height(ttk::Typeface &type) const {
    double tall = _label->natural_height(type, 0.0);

    if (_description->visible()) {
        tall += 3.0 + _description->natural_height(type, 0.0);
    }

    return tall;
}

// Rows in one box share a text column, so their fields line up and they stack together.
double SettingRow::column(ttk::Typeface &type) const {
    if (parent() == nullptr) {
        return texts_width(type);
    }

    double widest = 0.0;

    for (const Ptr &child : parent()->children()) {
        if (const auto *row = dynamic_cast<const SettingRow *>(child.get())) {
            widest = std::max(widest, row->texts_width(type));
        }
    }

    return widest;
}

bool SettingRow::stacked(ttk::Typeface &type, const double width) const {
    return width - column(type) - GAP < _field->minWidth;
}

double SettingRow::narrowest(ttk::Typeface &type) const {
    return std::max(texts_width(type), _field->minWidth);
}

double SettingRow::natural_height(ttk::Typeface &type, const double width) {
    const double texts = texts_height(type);

    if (stacked(type, width)) {
        return texts + BELOW + _field->natural_height(type, width) + 26.0;
    }

    return std::max(texts, ttk::Theme::control) + 26.0;
}

void SettingRow::place_texts(ttk::Typeface &type, double y, const double width) {
    const double name = _label->natural_height(type, width);

    _label->place(BLRect{_box.x, y, width, name}, type);
    y += name + 3.0;

    if (_description->visible()) {
        _description->place(BLRect{_box.x, y, width, _description->natural_height(type, width)}, type);
    }
}

void SettingRow::arrange(ttk::Typeface &type) {
    const double tall = texts_height(type);
    const double middle = _box.y + (_box.h / 2.0) - 3.0;

    if (stacked(type, _box.w)) {
        const double field = _field->natural_height(type, _box.w);
        const double top = middle - ((tall + BELOW + field) / 2.0);

        place_texts(type, top, _box.w);
        _field->place(BLRect{_box.x, top + tall + BELOW, _box.w, field}, type);

        return;
    }

    const double wide = std::min(CONTROL_WIDTH, _box.w - column(type) - GAP);
    const double field = _field->natural_height(type, wide);

    place_texts(type, middle - (tall / 2.0), _box.w - wide - GAP);
    _field->place(BLRect{_box.x + _box.w - wide, middle - (field / 2.0), wide, field}, type);
}

void SettingRow::paint(const ttk::Painter &painter) {
    Widget::paint(painter);

    if (!_last) {
        painter.fill(BLRect{_box.x, _box.y + _box.h - 1.0, _box.w, 1.0},
                     ttk::Theme::alpha(ttk::Theme::palette().border, 0.6));
    }
}
