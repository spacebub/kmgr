// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "gui/dialogs/AboutDialog.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <blend2d/blend2d.h>

#include "core/Program.h"
#include "ttk/draw/Glyphs.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/controls/Button.h"
#include "ttk/toolkit/controls/GlyphButton.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/toolkit/controls/TextView.h"
#include "ttk/toolkit/layout/Box.h"
#include "ttk/toolkit/layout/Picture.h"
#include "ttk/toolkit/layout/Scroll.h"
#include "ttk/toolkit/layout/Spacer.h"
#include "ttk/util/Desktop.h"

namespace Embedded {

extern const unsigned char authors[];
extern const std::size_t authorsSize;
extern const unsigned char mark128[];
extern const std::size_t mark128Size;

}

namespace {

struct Credits {
    std::vector<std::string> rows;
    std::vector<bool> headings;
};

Credits credits() {
    const std::string_view text{reinterpret_cast<const char *>(Embedded::authors), Embedded::authorsSize};
    Credits out;

    for (std::size_t at = 0; at < text.size();) {
        const std::size_t end = std::min(text.find('\n', at), text.size());
        std::string_view line = text.substr(at, end - at);

        at = end + 1;

        if (line.ends_with('\r')) {
            line.remove_suffix(1);
        }

        if (line.starts_with("# ") || (line.empty() && out.rows.empty())) {
            continue;
        }

        const bool heading = line.starts_with("## ");
        const std::size_t indent = std::min(line.find_first_not_of(' '), line.size());

        if (heading) {
            out.rows.emplace_back(line.substr(3));
        } else if (line.substr(indent).starts_with("- ")) {
            out.rows.push_back(std::string(indent + 2, ' ').append(line.substr(indent + 2)));
        } else {
            out.rows.emplace_back(line);
        }

        out.headings.push_back(heading);
    }

    return out;
}

BLImage mark() {
    BLImage image;

    if (image.read_from_data(Embedded::mark128, Embedded::mark128Size) != BL_SUCCESS) {
        return {};
    }

    return image;
}

}

AboutDialog::AboutDialog() {
    wanted = 520.0;

    ttk::Box *column = card()->append(ttk::Box::column());

    column->pad(22.0)->spacing(16.0);

    ttk::Box *top = column->append(ttk::Box::row());

    top->spacing(14.0)->cross(ttk::Box::Place::Centre);
    top->fixedHeight = 58.0;

    ttk::Picture *picture = top->append(std::make_unique<ttk::Picture>(mark()));

    picture->fixedWidth = 56.0;
    picture->fixedHeight = 56.0;

    ttk::Box *said = top->append(ttk::Box::column());

    said->spacing(2.0)->align(ttk::Box::Place::Centre);
    said->stretch = 1.0;

    said->append(std::make_unique<ttk::Label>("KernelManager"))
            ->font(ttk::Theme::palette().headingWeight, ttk::Theme::fontDisplay)
            ->tone(&ttk::Theme::Palette::text);

    said->append(
                std::make_unique<ttk::Label>("Version " + Program::version() + " · tinytk " + KERNELMGR_TINYTK_VERSION))
            ->font(400, ttk::Theme::fontSmall)
            ->tone(&ttk::Theme::Palette::faint);

    said->append(std::make_unique<ttk::Label>(
                         "Downloads, patches, configures, builds and installs kernels from kernel.org."))
            ->font(400, ttk::Theme::fontSmall)
            ->tone(&ttk::Theme::Palette::muted);

    ttk::GlyphButton *shut = top->append(std::make_unique<ttk::GlyphButton>(ttk::Glyphs::Glyph::Close, [this] {
        if (dismissed) {
            dismissed();
        }
    }));

    shut->size(26.0);
    shut->fixedWidth = 26.0;

    column->append(std::make_unique<ttk::Spacer>(0.0))->fixedHeight = 1.0;

    ttk::Box *rights = column->append(ttk::Box::column());

    rights->spacing(3.0);

    rights->append(std::make_unique<ttk::Label>("COPYRIGHT"))->section();
    rights->append(std::make_unique<ttk::Label>("© 2026 spacebub"))
            ->font(400, ttk::Theme::fontSmall)
            ->tone(&ttk::Theme::Palette::muted);

    ttk::Box *people = column->append(ttk::Box::column());

    people->spacing(4.0);

    people->append(std::make_unique<ttk::Label>("AUTHORS"))->section();

    ttk::Scroll *scroll = people->append(std::make_unique<ttk::Scroll>());

    scroll->fixedHeight = 150.0;

    auto *list = static_cast<ttk::TextView *>(scroll->hold(std::make_unique<ttk::TextView>()));
    Credits names = credits();

    list->face(400, ttk::Theme::fontSmall)->ink([headings = std::move(names.headings)](const std::size_t row) {
        const ttk::Theme::Palette &palette = ttk::Theme::palette();

        return row < headings.size() && headings[row] ? palette.text : palette.muted;
    });
    list->set_rows(std::move(names.rows));

    ttk::Box *row = column->append(ttk::Box::row());

    row->spacing(8.0);
    row->fixedHeight = ttk::Theme::controlSmall;

    ttk::GlyphButton *site = row->append(std::make_unique<ttk::GlyphButton>(
            ttk::Glyphs::Glyph::Globe, [] { ttk::Desktop::open("https://github.com/spacebub/kernelmgr"); }));

    site->outlined()->tooltip("Project page");
    site->fixedWidth = ttk::Theme::controlSmall;

    row->append(std::make_unique<ttk::Spacer>());

    row->append(std::make_unique<ttk::Button>("Close",
                                              [this] {
                                                  if (dismissed) {
                                                      dismissed();
                                                  }
                                              }))
            ->kind(ttk::Button::Kind::Primary)
            ->compact();
}
