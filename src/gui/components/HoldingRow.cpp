// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "gui/app/Reach.h"
#include "gui/components/HoldingRow.h"
#include "gui/components/Parts.h"
#include "gui/components/Wash.h"
#include "gui/model/Format.h"
#include "ttk/draw/Glyphs.h"
#include "ttk/draw/Theme.h"
#include "ttk/draw/Typeface.h"

HoldingRow::HoldingRow(Reach *reach, Log::Holding holding, std::function<void()> toggled)
        : _reach(reach), _holding(std::move(holding)), _toggled(std::move(toggled)) {
    const ttk::Theme::Palette &palette = ttk::Theme::of();

    _takesPointer = true;
    cursor = ttk::Cursor::Pointer;

    _folder = append(Parts::glyph_button(ttk::Glyphs::Glyph::Folder, 28.0, "Open these in your file manager",
                                         palette.faint, palette.accent, palette.accentSoft,
                                         [this] { _reach->logs.open(_holding.name); }));

    _bin = append(Parts::glyph_button(ttk::Glyphs::Glyph::Trash, 28.0, "Delete everything filed under this name",
                                      palette.faint, palette.danger, palette.dangerSoft, [this] {
        _reach->ask("Delete the logs for " + _holding.name + "?",
                    Format::size(_holding.size) + " over " + Format::runs(_holding.runs)
                        + " is removed. Nothing filed under any other name is touched.",
                    "Delete them", true, [this] { _reach->logs.remove_build(_holding.name); });
    }));
}

void HoldingRow::set_open(const bool open) {
    if (_open != open) {
        _open = open;
        invalidate();
    }
}

void HoldingRow::set_idle(const bool idle) const {
    _bin->set_enabled(idle);
}

void HoldingRow::arrange(ttk::Typeface &type) {
    double x = _box.x + _box.w - 6.0 - 28.0;

    _bin->place(BLRect{x, _box.y + ((_box.h - 28.0) / 2.0), 28.0, 28.0}, type);
    x -= 8.0 + 28.0;
    _folder->place(BLRect{x, _box.y + ((_box.h - 28.0) / 2.0), 28.0, 28.0}, type);
}

void HoldingRow::paint(const ttk::Painter &painter) {
    const ttk::Theme::Palette &palette = ttk::Theme::of();

    Wash::paint(painter, _box, ttk::Theme::radiusSmall, _open ? 1.0 : 0.0, holds_pointer() ? 1.0 : 0.0);

    constexpr float weight = 1.2F;
    const double side = ttk::Glyphs::span(weight);
    double x = _box.x + 10.0;

    ttk::Glyphs::draw(painter.context(), _open ? ttk::Glyphs::Glyph::Up : ttk::Glyphs::Glyph::Down,
                      BLPoint{x, _box.y + ((_box.h - side) / 2.0)}, weight, _open ? palette.accent : palette.faint);
    x += side + 8.0;

    const BLFont &name = painter.font(ttk::Typeface::pick(600, true), ttk::Theme::fontSmall);
    const double named = painter.width(name, _holding.name);

    painter.label(name, BLRect{x, _box.y, named + 2.0, _box.h}, ttk::Align::Start, _holding.name, palette.text);
    x += named + 8.0;

    const BLFont &small = painter.font(400, ttk::Theme::fontTiny);
    const BLFont &mono = painter.font(ttk::Typeface::mono, ttk::Theme::fontTiny);
    const std::string size = Format::size(_holding.size);
    const double sized = painter.width(mono, size);
    const double right = _folder->box().x - 8.0;

    painter.label(mono, BLRect{right - sized - 2.0, _box.y, sized + 2.0, _box.h}, ttk::Align::Start, size,
                  palette.muted);
    painter.label(small, BLRect{x, _box.y, std::max(0.0, right - sized - 8.0 - x), _box.h}, ttk::Align::Start,
                  Format::runs(_holding.runs), palette.faint);

    Widget::paint(painter);
}

bool HoldingRow::press(const ttk::Pointer & /*at*/) {
    return true;
}

void HoldingRow::release(const ttk::Pointer &at) {
    if (holds(at.x, at.y) && _toggled) {
        _toggled();
    }
}

void HoldingRow::enter() {
    Widget::enter();
    invalidate();
}

void HoldingRow::leave() {
    Widget::leave();
    invalidate();
}
