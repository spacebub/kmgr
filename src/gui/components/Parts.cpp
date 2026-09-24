// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "gui/app/Desk.h"
#include "gui/app/Reach.h"
#include "gui/components/Parts.h"

namespace Parts {

    std::unique_ptr<ttk::Label> section(const std::string &text) {
        auto made = std::make_unique<ttk::Label>(text);

        made->section();

        return made;
    }

    std::unique_ptr<ttk::Label> text(const std::string &text, const int weight, const float size,
                                     const ttk::Theme::Tone tone) {
        auto made = std::make_unique<ttk::Label>(text);

        made->font(weight, size)->tone(tone);

        return made;
    }

    std::unique_ptr<ttk::Label> note(const std::string &text) {
        auto made = std::make_unique<ttk::Label>(text);

        made->font(400, ttk::Theme::fontSmall)->tone(&ttk::Theme::Palette::faint)->wrap();

        return made;
    }

    std::unique_ptr<ttk::Label> path(Reach *reach, const std::string &path, const bool clickable,
                                     const std::string &opens) {
        auto made = std::make_unique<ttk::Label>(path);

        made->font(400, ttk::Theme::fontTiny)->tone(&ttk::Theme::Palette::faint)->path();

        if (!clickable) {
            made->hint = Desk::pretty(path);

            return made;
        }

        const std::string destination = opens.empty() ? path : opens;

        made->hint = "Open " + Desk::pretty(destination);
        made->on_click([reach, destination] {
            if (!Desk::reveal(destination)) {
                reach->notify.warning("Nothing on this system offered to open it.");
            }
        });

        return made;
    }

    std::unique_ptr<ttk::Pill> pill(const std::string &text, const ttk::Theme::Tone tone,
                                    const ttk::Theme::Tone wash, const bool dot) {
        auto made = std::make_unique<ttk::Pill>(text);

        made->tones(tone, wash)->dot(dot);

        return made;
    }

    std::unique_ptr<ttk::GlyphButton> glyph_button(const ttk::Glyphs::Glyph glyph, const double size,
                                                   const std::string &hint, const ttk::Theme::Tone rest,
                                                   const ttk::Theme::Tone hot, const ttk::Theme::Tone wash,
                                                   std::function<void()> clicked) {
        auto made = std::make_unique<ttk::GlyphButton>(glyph, std::move(clicked));

        made->size(size)->tone(rest, hot)->wash(wash)->tooltip(hint);
        made->fixedWidth = size;
        made->fixedHeight = size;

        return made;
    }

    std::unique_ptr<ttk::Box> above(const double top, ttk::Widget::Ptr child) {
        std::unique_ptr<ttk::Box> made = ttk::Box::column();

        made->pad(0.0, top, 0.0, 0.0);
        made->add(std::move(child));

        return made;
    }

    std::unique_ptr<ttk::Box> centred(ttk::Widget::Ptr child) {
        std::unique_ptr<ttk::Box> made = ttk::Box::row();

        made->align(ttk::Box::Place::Centre)->cross(ttk::Box::Place::Centre);
        made->add(std::move(child));

        return made;
    }

    bool shown(const ttk::Widget *widget) {
        for (const ttk::Widget *up = widget; up != nullptr; up = up->parent()) {
            if (!up->visible()) {
                return false;
            }
        }

        return true;
    }

    Tones lit(const bool on, const ttk::Theme::Tone tone, const ttk::Theme::Tone wash) {
        return on ? Tones{tone, wash} : Tones{&ttk::Theme::Palette::faint, &ttk::Theme::Palette::mutedSoft};
    }

}
