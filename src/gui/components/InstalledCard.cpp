// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "gui/components/InstalledCard.h"
#include "gui/components/Parts.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/layout/Box.h"
#include "ttk/toolkit/layout/Wrap.h"

namespace {
    using Palette = ttk::Theme::Palette;
}

InstalledCard::InstalledCard(const Catalog::Build &kernel, std::function<void(const std::string &)> action)
        : _action(std::move(action)), _name(kernel.name) {
    const ttk::Theme::Palette &palette = ttk::Theme::palette();

    inset = true;

    ttk::Box *layout = append(ttk::Box::column());
    layout->pad(16.0)->spacing(10.0);

    ttk::Box *head = layout->append(ttk::Box::row());
    head->spacing(8.0)->cross(ttk::Box::Place::Centre);

    _title = head->append(Parts::text(kernel.name, palette.headingWeight, ttk::Theme::fontLarge, &Palette::text));
    _title->stretch = 1.0;

    _running = head->append(Parts::pill("running now", &Palette::success, &Palette::successSoft));

    ttk::Wrap *pills = layout->append(std::make_unique<ttk::Wrap>());
    pills->spacing(6.0, 6.0);

    _modules = pills->append(Parts::pill("modules", &Palette::accent, &Palette::accentSoft));
    _image = pills->append(Parts::pill("image", &Palette::accent, &Palette::accentSoft));
    _initramfs = pills->append(Parts::pill("initramfs", &Palette::accent, &Palette::accentSoft));
    _signed = pills->append(Parts::pill("signed", &Palette::success, &Palette::successSoft));

    ttk::Wrap *buttons = layout->append(std::make_unique<ttk::Wrap>());
    buttons->spacing(6.0, 6.0);

    // Only worth offering when there is nothing left to rebuild from.
    _download = buttons->append(std::make_unique<ttk::Button>("Download", [this] { _action("download"); }));
    _download->kind(ttk::Button::Kind::Primary)->compact()
        ->tooltip("Fetch the archive this kernel came from, to have something to rebuild it from");

    _sign = buttons->append(std::make_unique<ttk::Button>("Sign", [this] { _action("sign"); }));
    _sign->compact();

    _remove = buttons->append(std::make_unique<ttk::Button>("Remove", [this] { _action("remove"); }));
    _remove->kind(ttk::Button::Kind::Danger)->compact();
}

void InstalledCard::sync(const Catalog::Build &kernel, const bool idle, const bool signable, const bool archived,
                         const std::string &version) {
    _title->set_text(kernel.name);
    _running->set_visible(kernel.running);

    const auto tint = [](ttk::Pill *pill, const Parts::Tones &tones) {
        pill->tones(tones.tone, tones.wash);
        pill->invalidate();
    };

    tint(_modules, Parts::lit(kernel.modules, &Palette::accent, &Palette::accentSoft));
    tint(_image, Parts::lit(kernel.image, &Palette::accent, &Palette::accentSoft));
    tint(_initramfs, Parts::lit(kernel.initramfs, &Palette::accent, &Palette::accentSoft));
    tint(_signed, Parts::lit(kernel.signedImage, &Palette::success, &Palette::successSoft));
    _signed->set_text(kernel.signedImage ? "signed" : "unsigned");

    _download->set_text("Download " + version);
    _download->set_visible(!archived && !kernel.extracted);
    _download->set_enabled(idle);

    _sign->tooltip(signable ? "Sign the installed image with sbctl so secure boot accepts it"
                            : "sbctl is not installed");
    _sign->set_enabled(idle && signable);

    _remove->tooltip(std::string("Delete its modules, boot files and mkinitcpio preset")
                     + (kernel.extracted ? ". The sources stay where they are" : ""));
    _remove->set_enabled(idle);
}
