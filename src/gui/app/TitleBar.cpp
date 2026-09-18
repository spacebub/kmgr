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

#include "gui/app/Look.h"
#include "gui/app/TitleBar.h"
#include "gui/components/Wash.h"
#include "ttk/draw/Theme.h"
#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Root.h"

namespace {
    constexpr double TAB_HEIGHT = 34.0;
    constexpr double TAB_SIDES = 28.0;
    constexpr double TAB_GAP = 2.0;

    ttk::Glyphs::Glyph shade_glyph() {
        switch (ttk::Theme::mode()) {
            case ttk::Theme::Mode::Light:
                return ttk::Glyphs::Glyph::Light;
            case ttk::Theme::Mode::Dark:
                return ttk::Glyphs::Glyph::Dark;
            default:
                return ttk::Glyphs::Glyph::System;
        }
    }

    std::string shade_hint() {
        const ttk::Theme::Mode &mode = ttk::Theme::mode();

        if (mode == ttk::Theme::Mode::System) {
            return "Following the desktop. Click for the light theme";
        }

        return mode == ttk::Theme::Mode::Light
            ? "Light theme. Click for the dark one"
            : "Dark theme. Click to follow the desktop again";
    }
}

TitleBar::TitleBar(Reach *reach, BLImage mark) : _reach(reach), _mark(std::move(mark)) {
    const ttk::Theme::Palette &palette = ttk::Theme::of();

    _takesPointer = true;

    _tabs.emplace_back(Page::Home, "Home");
    _tabs.emplace_back(Page::Kernels, "Kernels");
    _tabs.emplace_back(Page::Settings, "Settings");

    _shade = append(std::make_unique<ttk::GlyphButton>(shade_glyph(), [this] { _reach->cycleShade(); }));

    _minimize = append(std::make_unique<ttk::GlyphButton>(ttk::Glyphs::Glyph::Minimize,
                                                           [this] { _reach->window.minimize(); }));
    _maximize = append(std::make_unique<ttk::GlyphButton>(ttk::Glyphs::Glyph::Maximize,
                                                           [this] { _reach->window.toggle_maximize(); }));
    _close = append(std::make_unique<ttk::GlyphButton>(ttk::Glyphs::Glyph::Close,
                                                        [this] { _reach->window.stop(); }));
    _close->tone(palette.muted, BLRgba32(0xffffffff))->wash(palette.danger);
}

void TitleBar::sync(const Page page, const bool homeBadge, const std::string &trailing) {
    for (Tab &tab : _tabs) {
        const bool active = tab.key == page;

        if ((tab.on.value() > 0.5) != active) {
            tab.on.toward(active ? 1.0F : 0.0F, now(), 0.11, ttk::Anim::Curve::CubicOut);
            wake();
        }
    }

    if (_tabs[0].badge != homeBadge) {
        _tabs[0].badge = homeBadge;
        invalidate();
    }

    if (_trailing != trailing) {
        _trailing = trailing;

        if (root() != nullptr) {
            root()->relayout();
        }
    }

    _shade->glyph(shade_glyph());
    _shade->tooltip(shade_hint());
    _maximize->glyph(_reach->window.maximized() ? ttk::Glyphs::Glyph::Restore : ttk::Glyphs::Glyph::Maximize);
}

void TitleBar::arrange(ttk::Typeface &type) {
    const BLFont &face = type.at(400, ttk::Theme::fontBody);
    const double middle = _box.y + (Look::barHeight / 2.0);

    _brandEnd = 16.0 + 22.0 + (compact() ? 0.0 : 9.0 + type.width_tracked(face, "KernelManager", 0.2F));

    constexpr double side = ttk::Theme::controlSmall;
    double x = _box.x + _box.w - 8.0 - side;

    _close->place(BLRect{x, middle - (side / 2.0), side, side}, type);
    x -= side + TAB_GAP;
    _maximize->place(BLRect{x, middle - (side / 2.0), side, side}, type);
    x -= side + TAB_GAP;
    _minimize->place(BLRect{x, middle - (side / 2.0), side, side}, type);
    x -= 8.0 + side;
    _shade->place(BLRect{x, middle - (side / 2.0), side, side}, type);

    if (!compact() && !_trailing.empty()) {
        x -= 8.0 + type.width(type.at(400, ttk::Theme::fontSmall), _trailing);
    }

    double total = 0.0;

    for (Tab &tab : _tabs) {
        tab.width = type.width(type.at(tab.on.value() > 0.5 ? 600 : 400, ttk::Theme::fontBody), tab.label);
        total += tab.width + TAB_SIDES + TAB_GAP;
    }

    total = std::max(0.0, total - TAB_GAP);

    double at = std::max(_box.x + _brandEnd + 22.0, std::min(_box.x + ((_box.w - total) / 2.0), x - total - 16.0));

    for (Tab &tab : _tabs) {
        tab.box = BLRect{at, middle - (TAB_HEIGHT / 2.0), tab.width + TAB_SIDES, TAB_HEIGHT};
        at += tab.box.w + TAB_GAP;
    }
}

bool TitleBar::draggable(const double x, const double y) const {
    if (y >= _box.y + Look::barHeight) {
        return false;
    }

    if (tab_at(x, y) >= 0) {
        return false;
    }

    return x < _shade->box().x;
}

void TitleBar::paint(const ttk::Painter &painter) {
    const ttk::Theme::Palette &palette = ttk::Theme::of();
    const BLRect bar{_box.x, _box.y, _box.w, Look::barHeight};

    painter.fill(bar, palette.surface);
    painter.fill(BLRect{bar.x, bar.y + bar.h - 1.0, bar.w, 1.0}, palette.border);

    if (!_mark.is_empty()) {
        painter.context().blit_image(BLRect{bar.x + 16.0, bar.y + ((bar.h - 22.0) / 2.0), 22.0, 22.0}, _mark);
    }

    if (!compact()) {
        const BLFont &face = painter.font(400, ttk::Theme::fontBody);
        const double tall = painter.line_height(face);

        painter.tracked(face, BLPoint{bar.x + 16.0 + 22.0 + 9.0, bar.y + ((bar.h - tall) / 2.0)},
                        "KernelManager", palette.text, 0.2);
    }

    for (size_t index = 0; index < _tabs.size(); ++index) {
        const Tab &tab = _tabs[index];
        const double on = tab.on.value();
        const double lit = std::cmp_equal(index, _over) ? 1.0 : 0.0;

        Wash::paint(painter, tab.box, ttk::Theme::radiusSmall, on, lit);

        painter.label(painter.font(on > 0.5 ? 600 : 400, ttk::Theme::fontBody), tab.box, ttk::Align::Centre,
                      tab.label, on > 0.5 ? palette.accent : palette.muted);

        if (tab.badge && on <= 0.5) {
            painter.circle(BLPoint{tab.box.x + tab.box.w - 7.0 - 3.0, tab.box.y + 6.0 + 3.0}, 3.0, palette.accent);
        }
    }

    if (!compact() && !_trailing.empty()) {
        const BLFont &small = painter.font(400, ttk::Theme::fontSmall);
        const double wide = painter.width(small, _trailing);

        painter.label(small, BLRect{_shade->box().x - 8.0 - wide, bar.y, wide + 2.0, bar.h}, ttk::Align::Start,
                      _trailing, palette.faint);
    }

    Widget::paint(painter);
}

int TitleBar::tab_at(const double x, const double y) const {
    for (size_t index = 0; index < _tabs.size(); ++index) {
        if (const BLRect &box = _tabs[index].box;
            x >= box.x && x < box.x + box.w && y >= box.y && y < box.y + box.h) {
            return static_cast<int>(index);
        }
    }

    return -1;
}

bool TitleBar::press(const ttk::Pointer &at) {
    return at.y < _box.y + Look::barHeight && !draggable(at.x, at.y);
}

void TitleBar::release(const ttk::Pointer &at) {
    if (const int index = tab_at(at.x, at.y); index >= 0) {
        _reach->go(_tabs[static_cast<size_t>(index)].key);
    }
}

void TitleBar::hover(const ttk::Pointer &at) {
    if (const int over = tab_at(at.x, at.y); over != _over) {
        _over = over;
        invalidate(BLRect{_box.x, _box.y, _box.w, Look::barHeight});
    }
}

void TitleBar::leave() {
    Widget::leave();
    _over = -1;
    invalidate(BLRect{_box.x, _box.y, _box.w, Look::barHeight});
}

bool TitleBar::advance(const double now) {
    bool live = false;

    for (Tab &tab : _tabs) {
        const bool running = tab.on.live() || tab.lit.live();

        tab.on.advance(now);
        tab.lit.advance(now);
        live = live || running;
    }

    if (live) {
        invalidate(BLRect{_box.x, _box.y, _box.w, Look::barHeight});
    }

    return live;
}
