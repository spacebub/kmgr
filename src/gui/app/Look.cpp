// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "core/Configuration.h"
#include "gui/app/Look.h"
#include "ttk/draw/Theme.h"

namespace {
    constexpr ttk::Theme::Palette DARK{
        .background = BLRgba32{0xff0e1116},
        .surface = BLRgba32{0xff151a22},
        .raised = BLRgba32{0xff1b2029},
        .sunken = BLRgba32{0xff0b0e13},
        .field = BLRgba32{0xff04060a},

        .hover = BLRgba32{0x14ffffff},
        .border = BLRgba32{0xff262f3c},
        .borderStrong = BLRgba32{0xff3d4b5d},

        .text = BLRgba32{0xffe8ecf4},
        .muted = BLRgba32{0xffb3bdcd},
        .faint = BLRgba32{0xff8d99ad},

        .accent = BLRgba32{0xff5b8cff},
        .accentHover = BLRgba32{0xff7ba2ff},
        .accentText = BLRgba32{0xffffffff},
        .accentSoft = BLRgba32{0xff1a2440},

        .mutedSoft = BLRgba32{0xff232b37},
        .success = BLRgba32{0xff43d391},
        .successSoft = BLRgba32{0xff12301f},
        .warning = BLRgba32{0xfff0b429},
        .warningSoft = BLRgba32{0xff33270c},
        .danger = BLRgba32{0xfff4657a},
        .dangerSoft = BLRgba32{0xff341219},

        .shadow = BLRgba32{0x66000000},
        .scrim = BLRgba32{0xa8030509},

        .headingWeight = 700,
        .dark = true,
    };

    constexpr ttk::Theme::Palette LIGHT{
        .background = BLRgba32{0xfff4f6fa},
        .surface = BLRgba32{0xffffffff},
        .raised = BLRgba32{0xffffffff},
        .sunken = BLRgba32{0xffeef1f7},
        .field = BLRgba32{0xffe2e7f0},

        .hover = BLRgba32{0x140a1428},
        .border = BLRgba32{0xffdde3ee},
        .borderStrong = BLRgba32{0xffc3ccdd},

        .text = BLRgba32{0xff141922},
        .muted = BLRgba32{0xff4d5769},
        .faint = BLRgba32{0xff67738a},

        .accent = BLRgba32{0xff3568f0},
        .accentHover = BLRgba32{0xff2a58d8},
        .accentText = BLRgba32{0xffffffff},
        .accentSoft = BLRgba32{0xffe6edff},

        .mutedSoft = BLRgba32{0xffe3e8f1},
        .success = BLRgba32{0xff0a7d4e},
        .successSoft = BLRgba32{0xffe2f7ee},
        .warning = BLRgba32{0xff96620f},
        .warningSoft = BLRgba32{0xfffdf2dc},
        .danger = BLRgba32{0xffc62d46},
        .dangerSoft = BLRgba32{0xfffde8eb},

        .shadow = BLRgba32{0x1f12203a},
        .scrim = BLRgba32{0x6e0c1420},

        .headingWeight = 600,
        .dark = false,
    };

    ttk::Theme::Mode mode_of(const std::string &name) {
        if (name == "light") {
            return ttk::Theme::Mode::Light;
        }

        return name == "dark" ? ttk::Theme::Mode::Dark : ttk::Theme::Mode::System;
    }

    std::string name_of(const ttk::Theme::Mode mode) {
        if (mode == ttk::Theme::Mode::Light) {
            return "light";
        }

        return mode == ttk::Theme::Mode::Dark ? "dark" : "system";
    }
}

namespace Look {

    void install() {
        ttk::Theme::set_palettes(DARK, LIGHT);
        ttk::Theme::set_mode(mode_of(Configuration::get()->theme));
    }

    std::string mode_name() {
        return name_of(ttk::Theme::mode());
    }

    // A shade that cannot be saved only warns. It is not worth stopping the application over.
    void cycle(ttk::Notifier *notifier) {
        const ttk::Theme::Mode next = ttk::Theme::next_mode();

        ttk::Theme::set_mode(next);

        Settings settings = *Configuration::get();
        settings.theme = name_of(next);

        Configuration::set(settings);

        try {
            Configuration::save();
        } catch (const std::exception &ex) {
            notifier->warning(ex.what(), "Could not save the theme");
        }
    }

}
