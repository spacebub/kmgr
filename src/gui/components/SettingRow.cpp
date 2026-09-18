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

SettingRow::SettingRow(Reach *reach, const std::string &label, const std::string &description,
                       std::function<void(const std::string &)> edited) {
    _label = append(std::make_unique<ttk::Label>(label));
    _label->font(600, ttk::Theme::fontBody)->tone(ttk::Theme::of().text);

    _description = append(std::make_unique<ttk::Label>(description));
    _description->font(400, ttk::Theme::fontSmall)->tone(ttk::Theme::of().faint)->wrap();
    _description->set_visible(!description.empty());

    _field = append(std::make_unique<PathField>(reach, "", std::move(edited)));
}

SettingRow *SettingRow::browse(const std::string &title) {
    _field->browse(title, { "*" }, true);

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

double SettingRow::texts_height(ttk::Typeface &type, const double width) const {
    double tall = _label->natural_height(type, width);

    if (_description->visible()) {
        tall += 3.0 + _description->natural_height(type, width);
    }

    return tall;
}

double SettingRow::natural_height(ttk::Typeface &type, const double width) {
    const double texts = std::max(0.0, width - CONTROL_WIDTH - 24.0);

    return std::max(texts_height(type, texts), ttk::Theme::control) + 26.0;
}

void SettingRow::arrange(ttk::Typeface &type) {
    const double texts = std::max(0.0, _box.w - CONTROL_WIDTH - 24.0);
    const double tall = texts_height(type, texts);
    const double middle = _box.y + (_box.h / 2.0) - 3.0;

    double y = middle - (tall / 2.0);
    const double name = _label->natural_height(type, texts);

    _label->place(BLRect{_box.x, y, texts, name}, type);
    y += name + 3.0;

    if (_description->visible()) {
        _description->place(BLRect{_box.x, y, texts, _description->natural_height(type, texts)}, type);
    }

    const double wide = std::min(CONTROL_WIDTH, _box.w * 0.55);
    const double field = _field->natural_height(type, wide);

    _field->place(BLRect{_box.x + _box.w - wide, middle - (field / 2.0), wide, field}, type);
}

void SettingRow::paint(const ttk::Painter &painter) {
    Widget::paint(painter);

    if (!_last) {
        painter.fill(BLRect{_box.x, _box.y + _box.h - 1.0, _box.w, 1.0},
                     ttk::Theme::alpha(ttk::Theme::of().border, 0.6));
    }
}
