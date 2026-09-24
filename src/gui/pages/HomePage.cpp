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
#include "gui/pages/HomePage.h"
#include "ttk/draw/Theme.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/layout/Panel.h"
#include "ttk/toolkit/layout/Spacer.h"

namespace {
    using Palette = ttk::Theme::Palette;

    const ttk::Theme::Palette &palette() {
        return ttk::Theme::palette();
    }
}

void HomePage::Arrow::paint(const ttk::Painter &painter) {
    BLPath shape;

    shape.move_to(_box.x + 1.5, _box.y + 7.5);
    shape.line_to(_box.x + 27.5, _box.y + 7.5);
    shape.move_to(_box.x + 20.5, _box.y + 1.5);
    shape.line_to(_box.x + 27.5, _box.y + 7.5);
    shape.line_to(_box.x + 20.5, _box.y + 13.5);

    painter.context().set_stroke_caps(BL_STROKE_CAP_ROUND);
    painter.context().set_stroke_join(BL_STROKE_JOIN_ROUND);
    painter.stroke(shape, 2.2, palette().accent);
}

HomePage::HomePage(Reach *reach) : Box(Flow::Column), _reach(reach) {
    spacing(14.0);

    ttk::Panel *hero = append(std::make_unique<ttk::Panel>());
    ttk::Box *head = hero->append(ttk::Box::column());
    head->pad(28.0)->spacing(12.0);

    head->append(Parts::centred(Parts::section("RUNNING KERNEL")));

    ttk::Box *versions = head->append(ttk::Box::row());
    versions->spacing(16.0)->align(Place::Centre)->cross(Place::Centre);

    _current = versions->append(Parts::text("", 700, ttk::Theme::fontHero, &Palette::text));
    _current->on_click([this] { _reach->show(_reach->system.current()); });

    _arrow = versions->append(std::make_unique<Arrow>());
    _latest = versions->append(Parts::text("", 700, ttk::Theme::fontHero, &Palette::accent));

    auto facts = Parts::text("", 400, ttk::Theme::fontBody, &Palette::faint);

    _facts = facts.get();
    head->append(Parts::centred(std::move(facts)));

    head->append(Parts::above(4.0, std::make_unique<Parts::Rule>()));

    ttk::Box *news = head->append(ttk::Box::row());
    news->spacing(16.0)->align(Place::Centre)->cross(Place::Centre);
    news->fixedHeight = ttk::Theme::control;

    // Only shown when the versions above do not already say it.
    _status = news->append(Parts::pill("", &Palette::success, &Palette::successSoft));

    _ahead = news->append(Parts::text("Nothing newer has been released.", 400, ttk::Theme::fontBody,
                                      &Palette::muted));

    _retry = news->append(std::make_unique<ttk::Button>("Retry", [this] { _reach->system.check_latest(); }));
    _retry->tooltip("Ask kernel.org again");

    _update = news->append(std::make_unique<ttk::Button>("Update", [this] {
        const SystemStatus &system = _reach->system;

        _reach->ask("Update to " + system.latest() + "?",
                    system.latest() + (system.suffix().empty() ? "" : "-" + system.suffix())
                        + " is downloaded, built and installed, then " + system.release()
                        + " is removed. Steps that touch /boot and /usr/lib ask for your password.",
                    "Start the update", false, [this] { _reach->workflow.update(); });
    }));
    _update->kind(ttk::Button::Kind::Primary);

    ttk::Box *below = append(ttk::Box::row());
    below->spacing(14.0);
    below->stretch = 1.0;

    ttk::Panel *machine = below->append(std::make_unique<ttk::Panel>());
    machine->stretch = 1.0;

    ttk::Box *listing = machine->append(ttk::Box::column());
    listing->pad(20.0)->spacing(12.0);

    ttk::Box *caption = listing->append(ttk::Box::row());
    caption->cross(Place::Centre);
    caption->append(Parts::section("ON THIS MACHINE"))->stretch = 1.0;
    caption->append(std::make_unique<ttk::Button>("Open Kernels", [this] { _reach->show(""); }))
        ->kind(ttk::Button::Kind::Ghost)->compact();

    _empty = listing->append(std::make_unique<EmptyState>(
        "Nothing here yet", "Kernels lets you fetch a version from kernel.org and build it."));

    _preview = listing->append(std::make_unique<VersionList>(true, [this](const int row) {
        if (const Catalog::Entry *entry = _reach->catalog.at(row); entry != nullptr) {
            _reach->show(entry->name);
        }
    }));
    _preview->stretch = 1.0;

    ttk::Panel *places = below->append(std::make_unique<ttk::Panel>());
    places->fixedWidth = 310.0;

    ttk::Box *where = places->append(ttk::Box::column());
    where->pad(20.0)->spacing(16.0);
    where->append(Parts::section("WHERE THINGS LIVE"));

    _base = where->append(std::make_unique<ttk::Fact>("BASE DIRECTORY", ""));
    _base->path()->on_click("", [this] {
        if (!Desk::reveal(SystemStatus::base_directory())) {
            _reach->notify.warning("Nothing on this system offered to open it.");
        }
    });
    _base->fixedWidth = 268.0;

    ttk::Box *counts = where->append(ttk::Box::row());
    counts->spacing(18.0);

    _versions = counts->append(std::make_unique<ttk::Fact>("VERSIONS", "0"));
    _builds = counts->append(std::make_unique<ttk::Fact>("BUILDS", "0"));
    _archives = counts->append(std::make_unique<ttk::Fact>("ARCHIVES", "0"));

    where->append(std::make_unique<ttk::Spacer>());
    where->append(Parts::note("Patch sets and other repositories kept in the base directory can be pulled."));

    _pull = where->append(std::make_unique<ttk::Button>("Update repositories", [this] { _reach->workflow.pull(); }));
    _pull->tooltip("Run git pull in every repository in the base directory");
}

std::string HomePage::status() const {
    const SystemStatus &system = _reach->system;

    if (system.checking()) {
        return "Checking kernel.org";
    }

    if (!system.latest_known()) {
        return "Release feed unavailable";
    }

    if (system.update_available()) {
        return "Update available";
    }

    return system.ahead() ? "Ahead of the feed" : "Up to date";
}

void HomePage::sync() {
    const SystemStatus &system = _reach->system;
    const SettingsBridge &settings = _reach->settings;
    const Catalog &catalog = _reach->catalog;
    const bool update = system.update_available();

    _current->set_text(system.current());
    _current->hint = "Show " + system.current() + " under Kernels";
    _arrow->set_visible(update);
    _latest->set_visible(update);
    _latest->set_text(system.latest());

    const std::string jobs = std::to_string(settings.jobs > 0 ? settings.jobs : SettingsBridge::detected_jobs());

    _facts->set_text((system.suffix().empty() ? "no suffix" : system.suffix())
                     + "   ·   built with " + system.compiler()
                     + "   ·   " + (system.image_installed()
                                        ? (system.image_signed() ? "image signed" : "image unsigned")
                                        : "no image installed")
                     + "   ·   " + (system.secure_boot() ? "sbctl ready" : "no sbctl")
                     + "   ·   " + jobs + " build jobs");

    _status->set_visible(!update);
    _status->set_text(status());

    if (const std::string shape = system.current() + '\n' + system.latest() + '\n' + _facts->text() + '\n'
            + status() + (system.checking() ? "c" : "");
        shape != _shape) {
        _shape = shape;

        if (root() != nullptr) {
            root()->relayout();
        }
    }

    const Parts::Tones tones = system.checking()       ? Parts::Tones{&Palette::muted, &Palette::mutedSoft}
                             : !system.latest_known()  ? Parts::Tones{&Palette::warning, &Palette::warningSoft}
                                                       : Parts::Tones{&Palette::success, &Palette::successSoft};
    _status->tones(tones.tone, tones.wash);
    _status->invalidate();

    _ahead->set_visible(system.ahead() && system.latest_known() && !system.checking());
    _retry->set_visible(!system.latest_known() && !system.checking());

    _update->set_visible(update || system.checking());
    _update->set_text(system.checking() ? "Checking" : "Update to " + system.latest());
    _update->tooltip(system.checking() ? "" : "Builds " + system.latest() + " and replaces " + system.release());
    _update->set_enabled(update && !_reach->workflow.running());
    _update->busy(system.checking());

    _empty->set_visible(catalog.count() == 0);

    if (_catalogSeen != catalog.revision()) {
        _catalogSeen = catalog.revision();

        std::vector<VersionList::Row> rows;

        for (const Catalog::Entry &entry : catalog.entries()) {
            rows.push_back(VersionList::Row{
                .version = entry.name,
                .summary = entry.summary(),
                .running = entry.running,
                .installed = static_cast<int>(entry.installed.size())
            });
        }

        _preview->set_rows(std::move(rows));
    }

    _base->set_value(SystemStatus::base_directory());
    _base->on_click("Open " + SystemStatus::base_directory(), [this] {
        if (!Desk::reveal(SystemStatus::base_directory())) {
            _reach->notify.warning("Nothing on this system offered to open it.");
        }
    });
    _versions->set_value(std::to_string(catalog.count()));
    _builds->set_value(std::to_string(catalog.build_count()));
    _archives->set_value(std::to_string(catalog.archive_count()));

    _pull->set_enabled(!_reach->workflow.running());
}
