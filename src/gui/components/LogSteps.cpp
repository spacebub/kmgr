// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "gui/app/Reach.h"
#include "gui/components/LogSteps.h"
#include "gui/components/Parts.h"
#include "gui/model/Format.h"
#include "ttk/draw/Theme.h"
#include "ttk/system/Text.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/controls/Label.h"

namespace {
    using Palette = ttk::Theme::Palette;
}

LogSteps::LogSteps(Reach *reach) : Box(Flow::Column), _reach(reach) {
    spacing(6.0);
}

std::string LogSteps::step_name(const std::string &operation) {
    if (operation.empty()) {
        return "Whole runs";
    }

    if (operation == "pull") {
        return "Sources update";
    }

    if (operation == "download") {
        return "Download";
    }

    if (operation == "extract") {
        return "Extract";
    }

    if (operation == "patch") {
        return "Patch";
    }

    if (operation == "revert") {
        return "Revert";
    }

    if (operation == "configure") {
        return "Configure";
    }

    if (operation == "build") {
        return "Build";
    }

    if (operation == "install") {
        return "Install";
    }

    if (operation == "sign") {
        return "Sign";
    }

    if (operation == "archive") {
        return "Delete archive";
    }

    if (operation == "sources") {
        return "Delete sources";
    }

    if (operation == "remove") {
        return "Removal";
    }

    return operation;
}

std::string LogSteps::step_phrase(const std::string &operation) {
    if (operation.empty()) {
        return "whole run";
    }

    if (operation == "sources") {
        return "source deletion";
    }

    if (operation == "archive") {
        return "archive deletion";
    }

    return ttk::Text::lower(step_name(operation));
}

void LogSteps::set_idle(const bool idle) {
    _idle = idle;

    for (ttk::Widget *bin : _bins) {
        bin->set_enabled(idle);
    }
}

void LogSteps::set_steps(const std::string &build, const std::vector<Log::Filed> &steps) {
    _build = build;
    _count = static_cast<int>(steps.size());
    _loaded = true;
    _bins.clear();
    clear();

    if (steps.empty()) {
        append(Parts::note("Nothing filed here yet."));
    }

    for (const Log::Filed &step : steps) {
        ttk::Box *row = append(ttk::Box::row());

        row->spacing(8.0)->cross(Place::Centre);
        row->append(Parts::text(step_name(step.operation), 400, ttk::Theme::fontSmall, &Palette::text));
        row->append(Parts::text(Format::runs(step.runs), 400, ttk::Theme::fontTiny, &Palette::faint))->stretch = 1.0;
        row->append(Parts::text(Format::size(step.size), 400, ttk::Theme::fontTiny, &Palette::muted))->mono();

        const std::string operation = step.operation;
        const std::string size = Format::size(step.size);
        const int runs = step.runs;

        ttk::GlyphButton *bin = row->append(Parts::glyph_button(
            ttk::Glyphs::Glyph::Trash, 26.0, "Delete what this step has filed here", &Palette::faint,
            &Palette::danger, &Palette::dangerSoft, [this, operation, size, runs] {
                _reach->ask("Delete the " + step_phrase(operation) + " logs for " + _build + "?",
                            size + " over " + Format::runs(runs)
                                + " is removed. Everything filed here under the other steps stays.",
                            "Delete them", true,
                            [this, operation] { _reach->logs.remove_step(_build, operation); });
            }));

        bin->set_enabled(_idle);
        _bins.push_back(bin);
    }

    if (root() != nullptr) {
        root()->relayout();
    }
}
