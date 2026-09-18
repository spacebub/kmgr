// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_COMPONENTS_BUILDCARD_H
#define KERNELMGR_GUI_COMPONENTS_BUILDCARD_H


#include <functional>
#include <string>

#include "gui/components/Choice.h"
#include "gui/components/Fold.h"
#include "gui/components/LogSteps.h"
#include "gui/components/PathField.h"
#include "gui/model/Catalog.h"
#include "ttk/toolkit/controls/Button.h"
#include "ttk/toolkit/controls/GlyphButton.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/toolkit/controls/Pill.h"
#include "ttk/toolkit/controls/Toggle.h"
#include "ttk/toolkit/layout/Panel.h"

struct Reach;

class BuildCard : public ttk::Panel {
public:
    BuildCard(Reach *reach, const Catalog::Build &build, std::function<void(const std::string &)> action);

    void sync(const Catalog::Build &build, bool idle, bool signable, bool expanded);

    [[nodiscard]] const std::string &name() const { return _build.name; }

private:
    // In switch order, spelled the way the settings file writes them.
    static constexpr const char *TOOLCHAINS[] = { "gcc", "llvm", "custom" };

    [[nodiscard]] static std::string toolchain_label(const std::string &name);
    [[nodiscard]] static int toolchain_index(const std::string &name);

    [[nodiscard]] bool tuned() const;
    [[nodiscard]] bool switching() const;
    [[nodiscard]] bool patchable() const;

    void remember() const;
    void read_logs();
    void resolve();
    void annotate() const;

    Reach *_reach;
    std::function<void(const std::string &)> _action;
    Catalog::Build _build;

    bool _idle = true;
    bool _signable = false;
    bool _expanded = false;

    // Nothing is written back before every control is up, or the first one to
    // settle would answer for the ones that are not there yet.
    bool _ready = false;

    int _logsRead = -1;
    std::string _resolvedConfig;
    std::string _resolvedPatch;
    std::string _logSize;

    ttk::Label *_title = nullptr;
    ttk::Pill *_running = nullptr;
    ttk::Pill *_options = nullptr;
    ttk::GlyphButton *_cog = nullptr;
    ttk::Label *_path = nullptr;
    ttk::Pill *_patched = nullptr;
    ttk::Pill *_configured = nullptr;
    ttk::Pill *_built = nullptr;
    ttk::Pill *_made = nullptr;
    Choice *_compiler = nullptr;
    Fold *_fold = nullptr;
    PathField *_config = nullptr;
    PathField *_patch = nullptr;
    ttk::Toggle *_errors = nullptr;
    ttk::Pill *_weight = nullptr;
    ttk::GlyphButton *_bin = nullptr;
    LogSteps *_steps = nullptr;
    ttk::Button *_everything = nullptr;
    ttk::Button *_upToBuild = nullptr;
    ttk::Button *_patchStep = nullptr;
    ttk::Button *_configure = nullptr;
    ttk::Button *_buildStep = nullptr;
    ttk::Button *_install = nullptr;
    ttk::Button *_sources = nullptr;
};


#endif //KERNELMGR_GUI_COMPONENTS_BUILDCARD_H
