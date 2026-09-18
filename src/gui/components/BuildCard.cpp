// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>

#include "gui/app/Desk.h"
#include "gui/app/Reach.h"
#include "gui/components/BuildCard.h"
#include "gui/components/Parts.h"
#include "ttk/draw/Theme.h"
#include "ttk/system/Text.h"
#include "ttk/toolkit/layout/Box.h"
#include "ttk/toolkit/layout/Spacer.h"
#include "ttk/toolkit/layout/Wrap.h"

namespace {
    const ttk::Theme::Palette &palette() {
        return ttk::Theme::of();
    }
}

std::string BuildCard::toolchain_label(const std::string &name) {
    if (name == "llvm") {
        return "LLVM";
    }

    return name == "custom" ? "Custom" : "GCC";
}

int BuildCard::toolchain_index(const std::string &name) {
    for (int index = 0; index < 3; ++index) {
        if (name == TOOLCHAINS[index]) {
            return index;
        }
    }

    return 0;
}

BuildCard::BuildCard(Reach *reach, const Catalog::Build &build, std::function<void(const std::string &)> action)
        : _reach(reach), _action(std::move(action)), _build(build) {
    inset = true;

    ttk::Box *layout = append(ttk::Box::column());
    layout->pad(16.0)->spacing(10.0);

    // --- the head ---

    ttk::Box *head = layout->append(ttk::Box::row());
    head->spacing(8.0)->cross(ttk::Box::Place::Centre);

    _title = head->append(Parts::text(build.name, palette().headingWeight, ttk::Theme::fontLarge, palette().text));
    _title->stretch = 1.0;

    _running = head->append(Parts::pill("running now", palette().success, palette().successSoft));

    // Shut, it still has to say that it is carrying something.
    _options = head->append(Parts::pill("options set", palette().accent, palette().accentSoft));

    // 135 degrees is three of the cog's eight teeth, so it comes to rest looking as it started.
    _cog = head->append(Parts::glyph_button(ttk::Glyphs::Glyph::Cog, 30.0, "Options and logs for this build",
                                            palette().faint, palette().accent, palette().accentSoft,
                                            [this] { _action("options"); }));
    _cog->spin(135.0);

    _path = layout->append(Parts::path(reach, build.source, true));

    // --- state ---

    ttk::Box *state = layout->append(ttk::Box::row());
    state->spacing(10.0)->cross(ttk::Box::Place::Centre);

    ttk::Wrap *pills = state->append(std::make_unique<ttk::Wrap>());
    pills->spacing(6.0, 6.0);
    pills->stretch = 1.0;

    _patched = pills->append(Parts::pill("patched", palette().success, palette().successSoft));
    _configured = pills->append(Parts::pill("configured", palette().success, palette().successSoft));
    _built = pills->append(Parts::pill("built", palette().success, palette().successSoft));

    // Shown only when it disagrees with the switch, which otherwise says the same.
    _made = pills->append(Parts::pill("made with", palette().warning, palette().warningSoft));

    _compiler = state->append(std::make_unique<Choice>([this](const int index) {
        _compiler->set_current(index);
        remember();
    }));
    _compiler->set_options({ "GCC", "LLVM", "Custom" });
    _compiler->set_disabled_hint("No custom toolchain flags are set. The settings page takes them");

    // --- options ---

    _fold = layout->append(std::make_unique<Fold>());

    ttk::Box *options = _fold->body();

    options->append(std::make_unique<Parts::Rule>());

    _config = options->append(std::make_unique<PathField>(reach, "Configuration (optional)",
                                                          [this](const std::string &) {
        annotate();
        remember();
    }));
    _config->browse("Select a kernel configuration", { "*.config", "config*" }, false);
    _config->placeholder("chosen automatically");

    _patch = options->append(std::make_unique<PathField>(reach, "Patch definition (optional)",
                                                         [this](const std::string &) {
        annotate();
        remember();
    }));
    _patch->browse("Select a patch definition", { "*-patch.txt", "*.sh" }, false);
    _patch->placeholder("chosen automatically");

    _errors = options->append(std::make_unique<ttk::Toggle>("Continue after errors", [this](const bool value) {
        _errors->set_checked(value);
        remember();
    }));

    options->append(std::make_unique<Parts::Rule>());

    ttk::Box *logs = options->append(ttk::Box::row());
    logs->spacing(6.0)->cross(ttk::Box::Place::Centre);
    logs->append(Parts::section("LOGS"))->stretch = 1.0;

    _weight = logs->append(Parts::pill("none", palette().faint, palette().mutedSoft, false));

    logs->append(Parts::glyph_button(ttk::Glyphs::Glyph::Folder, 30.0, "Open this build's logs in your file manager",
                                     palette().faint, palette().accent, palette().accentSoft,
                                     [this] { _reach->logs.open(_build.name); }));

    _bin = logs->append(Parts::glyph_button(ttk::Glyphs::Glyph::Trash, 30.0, "Delete every log this build has written",
                                            palette().faint, palette().danger, palette().dangerSoft, [this] {
        _reach->ask("Delete the logs for " + _build.name + "?",
                    "Every run filed under this build is removed. Other builds of the same "
                    "version keep theirs, and nothing else is touched.",
                    "Delete them", true, [this] { _reach->logs.remove_build(_build.name); });
    }));

    _steps = options->append(std::make_unique<LogSteps>(reach));

    options->append(std::make_unique<ttk::Spacer>(0.0))->fixedHeight = 2.0;

    // --- the runs ---

    layout->append(Parts::above(2.0, Parts::section("IN ONE RUN")));

    ttk::Wrap *runs = layout->append(std::make_unique<ttk::Wrap>());
    runs->spacing(6.0, 6.0);

    _everything = runs->append(std::make_unique<ttk::Button>("Everything", [this] { _action("everything"); }));
    _everything->kind(ttk::Button::Kind::Primary)->compact();

    _upToBuild = runs->append(std::make_unique<ttk::Button>("Up to build", [this] { _action("upToBuild"); }));
    _upToBuild->compact()->tooltip("Patch, configure and build. Nothing is installed and nothing needs a password");

    layout->append(Parts::above(4.0, Parts::section("ONE STEP AT A TIME")));

    ttk::Wrap *steps = layout->append(std::make_unique<ttk::Wrap>());
    steps->spacing(6.0, 6.0);

    _patchStep = steps->append(std::make_unique<ttk::Button>("Patch", [this] {
        _action(_build.patched ? "revert" : "patch");
    }));
    _patchStep->compact();

    _configure = steps->append(std::make_unique<ttk::Button>("Configure", [this] { _action("configure"); }));
    _configure->compact()->tooltip(
        "Put a configuration in place, set the local version and answer new symbols with oldconfig");

    _buildStep = steps->append(std::make_unique<ttk::Button>("Build", [this] { _action("build"); }));
    _buildStep->compact()->tooltip("Run make over the whole tree");

    _install = steps->append(std::make_unique<ttk::Button>("Install", [this] { _action("install"); }));
    _install->compact()->tooltip(
        "Install modules, image and initramfs, run dkms and refresh grub. Needs a password");

    auto removing = std::make_unique<ttk::Wrap>();
    ttk::Wrap *removal = removing.get();

    removal->spacing(6.0, 6.0);
    layout->append(Parts::above(2.0, std::move(removing)));

    _sources = removal->append(std::make_unique<ttk::Button>("Delete sources", [this] { _action("sources"); }));
    _sources->kind(ttk::Button::Kind::Danger)->compact()
        ->tooltip("Remove the build directory only. Anything installed from it stays");

    // Where the panel was left, so opening it again opens it as it was.
    const BuildConfig::Choices chosen = WorkflowRunner::choices(build.version, build.suffix);

    _config->set_text(chosen.config);
    _patch->set_text(chosen.patch);
    _errors->set_checked(chosen.force);
    _compiler->set_current(toolchain_index(toolchain_name(chosen.compiler)));

    resolve();
    annotate();

    _ready = true;
}

bool BuildCard::tuned() const {
    return !_config->text().empty() || !_patch->text().empty() || _errors->checked;
}

// Custom flags can name either compiler or neither, so they are never held against the tree.
bool BuildCard::switching() const {
    const std::string chosen = TOOLCHAINS[std::clamp(_compiler->current(), 0, 2)];

    return !_build.toolchain.empty() && chosen != "custom" && _build.toolchain != chosen;
}

// Most trees have nothing to patch, so the word only shows once a patch is in or waiting.
bool BuildCard::patchable() const {
    return _build.patched || !_patch->text().empty() || !_resolvedPatch.empty();
}

void BuildCard::remember() const {
    if (!_ready) {
        return;
    }

    WorkflowRunner::set_choices(_build.version, _build.suffix, BuildConfig::Choices{
        .config = _config->text(),
        .patch = _patch->text(),
        .compiler = toolchain_from(TOOLCHAINS[std::clamp(_compiler->current(), 0, 2)]),
        .force = _errors->checked
    });

    _reach->touch();
}

void BuildCard::resolve() {
    _resolvedConfig = Desk::resolved_config(_build.version, _build.suffix);
    _resolvedPatch = Desk::resolved_patch(_build.version, _build.suffix);
}

void BuildCard::annotate() const {
    _config->note(!_config->text().empty() ? "Overriding what would be chosen"
                  : !_resolvedConfig.empty() ? "Using " + _resolvedConfig
                                             : "None found, a default configuration will be generated");
    _patch->note(!_patch->text().empty() ? "Overriding what would be chosen"
                 : !_resolvedPatch.empty() ? "Using " + _resolvedPatch
                                           : "None found, patching will be skipped");
}

void BuildCard::read_logs() {
    _logsRead = _reach->logs.revision();
    _logSize = Logs::build_size(_build.name);
    _weight->set_text(_logSize.empty() ? "none" : _logSize);
    _steps->set_steps(_build.name, Logs::build_logs(_build.name));
}

void BuildCard::sync(const Catalog::Build &build, const bool idle, const bool signable, const bool expanded) {
    const bool opened = expanded && !_expanded;

    if (build.source != _build.source) {
        _build = build;
        resolve();
        annotate();
    } else {
        _build = build;
    }

    _idle = idle;
    _signable = signable;
    _expanded = expanded;

    _title->set_text(build.name);
    _running->set_visible(build.running);
    _options->set_visible(tuned() && !expanded);

    _cog->tooltip(expanded ? "Hide this build's options and logs" : "Options and logs for this build");
    _cog->tone(expanded || tuned() ? palette().accent : palette().faint, palette().accent);

    if (opened || (!expanded && _fold->open())) {
        _cog->spun(expanded);
    }

    _path->set_text(build.source);

    const auto tint = [](ttk::Pill *pill, const Parts::Tones &tones) {
        pill->tones(tones.tone, tones.wash);
        pill->invalidate();
    };

    _patched->set_visible(patchable());
    _patched->set_text(build.patched ? "patched" : "not patched");
    tint(_patched, Parts::lit(build.patched, palette().success, palette().successSoft));

    _configured->set_text(build.configured ? "configured" : "not configured");
    tint(_configured, Parts::lit(build.configured, palette().success, palette().successSoft));

    _built->set_text(build.built ? "built" : "not built");
    tint(_built, Parts::lit(build.built, palette().success, palette().successSoft));

    _made->set_visible(switching());
    _made->set_text("made with " + toolchain_label(build.toolchain));

    // The saved flags rather than the form's, since those are what a run from here hands make.
    _compiler->set_disabled(ttk::Text::trim(_reach->settings.customFlags).empty() ? std::vector<int>{ 2 }
                                                                                   : std::vector<int>{});
    _compiler->set_hint(build.toolchain.empty()
        ? "What make is run with. Custom is the flags in the settings"
        : "This tree was made with " + toolchain_label(build.toolchain)
            + ". Building it with another one builds it over");

    _fold->set_open(expanded);

    if (expanded && _logsRead != _reach->logs.revision()) {
        read_logs();
    }

    _steps->set_idle(idle);
    _bin->set_enabled(idle && _steps->count() > 0);

    _everything->tooltip((build.built ? "Already built, so this installs it" : "Patch, configure, build and install")
                         + std::string(signable ? ", then sign" : "") + ", one step after another");
    _everything->set_enabled(idle);
    _upToBuild->set_enabled(idle);

    _patchStep->set_text(build.patched ? "Revert" : "Patch");
    _patchStep->tooltip(build.patched
        ? "Take the patches back out of the tree, in the order they went in"
        : "Apply the patches the patch definition names, from the kernel directory");
    _patchStep->set_enabled(idle);
    _configure->set_enabled(idle);
    _buildStep->set_enabled(idle);
    _install->set_enabled(idle);
    _sources->set_enabled(idle);
}
