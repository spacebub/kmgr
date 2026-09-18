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

#include "gui/app/Desk.h"
#include "gui/app/Reach.h"
#include "gui/components/Parts.h"
#include "gui/components/Wash.h"
#include "gui/pages/SettingsPage.h"
#include "ttk/draw/Theme.h"
#include "ttk/draw/Typeface.h"
#include "ttk/system/Text.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/layout/Spacer.h"

namespace {
    constexpr double RAIL_GAP = 18.0;
    constexpr double FOOTER_HEIGHT = 62.0;

    const ttk::Theme::Palette &palette() {
        return ttk::Theme::of();
    }

    std::unique_ptr<ttk::Panel> card(ttk::Box *&inside, const double spacing) {
        auto made = std::make_unique<ttk::Panel>();

        inside = made->append(ttk::Box::column());
        inside->pad(20.0)->spacing(spacing);

        return made;
    }

    std::unique_ptr<ttk::Box> captioned(const std::string &title, const std::string &about) {
        std::unique_ptr<ttk::Box> made = ttk::Box::column();

        made->spacing(3.0);
        made->append(Parts::text(title, 600, ttk::Theme::fontBody, palette().text));
        made->append(Parts::note(about));

        return made;
    }
}

SettingsPage::Rail::Rail(SettingsPage *page) : _page(page) {
    _takesPointer = true;
}

int SettingsPage::Rail::row_at(const double y) const {
    const double at = y - _box.y;

    if (at < 0.0) {
        return -1;
    }

    const auto row = static_cast<int>(at / (ROW + GAP));

    return row < 4 && at - (row * (ROW + GAP)) < ROW ? row : -1;
}

void SettingsPage::Rail::paint(const ttk::Painter &painter) {
    for (int index = 0; index < 4; ++index) {
        const BLRect line{_box.x, _box.y + (index * (ROW + GAP)), _box.w, ROW};
        const bool active = index == _page->_section;

        Wash::paint(painter, line, ttk::Theme::radiusSmall, active ? 1.0 : 0.0, index == _over ? 1.0 : 0.0);
        painter.label(painter.font(active ? 600 : 400, ttk::Theme::fontBody),
                      BLRect{line.x + 12.0, line.y, std::max(0.0, line.w - 20.0), line.h}, ttk::Align::Start,
                      SECTIONS[index], active ? palette().accent : palette().muted);
    }
}

ttk::Cursor SettingsPage::Rail::cursor_at(double /*x*/, const double y) const {
    return row_at(y) >= 0 ? ttk::Cursor::Pointer : ttk::Cursor::Default;
}

void SettingsPage::Rail::hover(const ttk::Pointer &at) {
    if (const int over = row_at(at.y); over != _over) {
        _over = over;
        invalidate();
    }
}

void SettingsPage::Rail::leave() {
    Widget::leave();
    _over = -1;
    invalidate();
}

bool SettingsPage::Rail::press(const ttk::Pointer &at) {
    return row_at(at.y) >= 0;
}

void SettingsPage::Rail::release(const ttk::Pointer &at) {
    if (const int row = row_at(at.y); row >= 0 && row != _page->_section) {
        _page->_section = row;
        _page->_reach->touch();
    }
}

SettingsPage::SwitchCard::SwitchCard(std::function<void()> flipped) : _flipped(std::move(flipped)) {
    _takesPointer = true;
    cursor = ttk::Cursor::Pointer;
    hoverable = true;
}

void SettingsPage::SwitchCard::release(const ttk::Pointer &at) {
    if (holds(at.x, at.y) && _flipped) {
        _flipped();
    }
}

void SettingsPage::SwitchCard::enter() {
    Widget::enter();
    lit = true;
    invalidate();
}

void SettingsPage::SwitchCard::leave() {
    Widget::leave();
    lit = false;
    invalidate();
}

SettingsPage::ToolchainRow::ToolchainRow(ttk::Widget::Ptr texts, ttk::Widget::Ptr choice) {
    _texts = add(std::move(texts));
    _choice = add(std::move(choice));
}

double SettingsPage::ToolchainRow::natural_height(ttk::Typeface &type, const double width) {
    return std::max(_texts->natural_height(type, std::max(0.0, width - 200.0)), ttk::Theme::control) + 26.0;
}

void SettingsPage::ToolchainRow::arrange(ttk::Typeface &type) {
    const double middle = _box.y + (_box.h / 2.0) - 3.0;
    const double texts = std::max(0.0, _box.w - 200.0);
    const double tall = _texts->natural_height(type, texts);
    const double wide = _choice->natural_width(type);
    const double high = _choice->natural_height(type, wide);

    _texts->place(BLRect{_box.x, middle - (tall / 2.0), texts, tall}, type);
    _choice->place(BLRect{_box.x + _box.w - wide, middle - (high / 2.0), wide, high}, type);
}

void SettingsPage::ToolchainRow::paint(const ttk::Painter &painter) {
    Widget::paint(painter);
    painter.fill(BLRect{_box.x, _box.y + _box.h - 1.0, _box.w, 1.0}, ttk::Theme::alpha(palette().border, 0.6));
}

SettingsPage::SettingsPage(Reach *reach) : _reach(reach) {
    const auto touched = [this](const std::string &) { _reach->touch(); };

    _rail = append(std::make_unique<Rail>(this));

    // Each group scrolls on its own, so a long one does not drag the rest with it.
    const auto pane = [this]() -> ttk::Box * {
        ttk::Scroll *scroll = append(std::make_unique<ttk::Scroll>());
        ttk::Box *stack = static_cast<ttk::Box *>(scroll->hold(ttk::Box::column()));

        stack->pad(0.0, 0.0, 12.0, 0.0)->spacing(14.0);
        _panes.push_back(scroll);

        return stack;
    };

    // --- general ---

    ttk::Box *general = pane();
    ttk::Box *inside = nullptr;

    general->append(card(inside, 10.0));
    inside->append(Parts::section("WHERE KERNELS ARE KEPT"));
    _base = inside->append(std::make_unique<SettingRow>(
        reach, "Base directory", "Archives are unpacked here and kernels are built here", touched));
    _base->browse("Select the base directory");
    _archives = inside->append(std::make_unique<SettingRow>(
        reach, "Archive directory", "Where the tarballs downloaded from kernel.org are stored", touched));
    _archives->browse("Select the archive directory")->last();

    general->append(card(inside, 10.0));
    inside->append(Parts::section("WHERE KERNELS ARE INSTALLED"));
    _boot = inside->append(std::make_unique<SettingRow>(
        reach, "Boot directory", "Images, initramfs, System.map and configs are installed here", touched));
    _boot->browse("Select the boot directory");
    _grub = inside->append(std::make_unique<SettingRow>(
        reach, "Grub directory", "grub.cfg is rewritten here after an install or a removal", touched));
    _grub->browse("Select the grub directory")->last();

    general->append(card(inside, 10.0));
    inside->append(Parts::section("DOWNLOADING"));
    _cdn = inside->append(std::make_unique<SettingRow>(reach, "Kernel CDN",
                                                       "Archives are fetched from here, under vMAJOR.x", touched));
    _format = inside->append(std::make_unique<SettingRow>(
        reach, "Archive format", "The extension asked for, tar.xz unless kernel.org offers something else",
        touched));
    _format->last();

    auto alerts = std::make_unique<SwitchCard>([this] {
        _wantedNotifications = !_wantedNotifications;
        _reach->touch();
    });
    inside = alerts->append(ttk::Box::column());
    inside->pad(20.0)->spacing(12.0);
    inside->append(Parts::section("NOTIFICATIONS"));

    ttk::Box *alertRow = inside->append(ttk::Box::row());
    alertRow->spacing(16.0)->cross(ttk::Box::Place::Centre);
    alertRow->append(captioned("Desktop notifications",
                               "Get a notification when a run asks for your password and when it finishes. "
                               "Nothing else is sent."))->stretch = 1.0;
    _notifications = alertRow->append(std::make_unique<ttk::Toggle>("", [this](const bool value) {
        _wantedNotifications = value;
        _reach->touch();
    }));
    general->append(std::move(alerts));

    // --- build ---

    ttk::Box *build = pane();

    build->append(card(inside, 10.0));
    inside->append(Parts::section("BUILD"));

    auto toolchain = std::make_unique<Choice>([this](const int index) {
        _wantedCompiler = TOOLCHAINS[index];
        _reach->touch();
    });
    _toolchain = toolchain.get();
    _toolchain->set_disabled_hint("Nothing to hand make yet. Fill in the custom toolchain flags below first");
    inside->append(std::make_unique<ToolchainRow>(
        captioned("Toolchain", "What a build is made with. Each build can be set to another one on its own "
                               "card. Custom hands make the flags below instead"),
        std::move(toolchain)));

    _jobs = inside->append(std::make_unique<SettingRow>(
        reach, "Build jobs",
        "How many jobs make is given. Left empty, one per core: " + std::to_string(SettingsBridge::detected_jobs())
            + " on this machine",
        touched));
    _jobs->placeholder(std::to_string(SettingsBridge::detected_jobs()) + " (detected)")->mono(false)->last();

    build->append(card(inside, 12.0));
    inside->append(Parts::section("CUSTOM TOOLCHAIN"));
    inside->append(Parts::note("What every make of a build set to Custom is handed, written the way it would be "
                               "typed after the word make. It stands in place of what GCC and LLVM carry rather "
                               "than joining it, and a shell reads the line, so quotes hold together and $HOME is "
                               "your home."));
    _flags = inside->append(std::make_unique<ttk::Field>("Make flags", touched));
    _flags->placeholder("nothing, which is a plain make")->mono();

    std::unique_ptr<ttk::Box> presetRow = ttk::Box::row();
    ttk::Box *presets = presetRow.get();

    presets->spacing(10.0)->cross(ttk::Box::Place::Centre);
    inside->append(Parts::above(2.0, std::move(presetRow)));
    presets->append(Parts::text("Start from", 400, ttk::Theme::fontSmall, palette().muted));
    presets->append(std::make_unique<ttk::Button>("AOCC", [this] {
        _flags->set_text(SettingsBridge::aocc_preset());
        _reach->touch();
    }))->compact()->tooltip("AMD's clang, with the flags that are the reason to reach for it");
    presets->append(std::make_unique<ttk::Spacer>());

    inside->append(Parts::note("AOCC brings its own LLVM, so the preset points LLVM= at the bin directory it was "
                               "unpacked into, trailing slash and all. That path is not yours yet: put your own "
                               "in before building with it."));

    // --- password ---

    ttk::Box *password = pane();

    password->append(card(inside, 12.0));
    inside->append(Parts::section("PASSWORD"));

    _elevation = inside->append(std::make_unique<SettingRow>(
        reach, "Elevation command", "What the steps that need root are run through, sudo unless it is changed",
        touched));

    ttk::Box *remembered = inside->append(ttk::Box::row());
    remembered->spacing(16.0)->cross(ttk::Box::Place::Centre);
    remembered->append(captioned("Remembered for this session",
                                 "Steps that write outside your home ask once and reuse the answer. It is held "
                                 "in memory only, never written anywhere."))->stretch = 1.0;
    _forget = remembered->append(std::make_unique<ttk::Button>("Forget it", [this] {
        _reach->settings.forget_password();
    }));
    _forget->glyph(ttk::Glyphs::Glyph::Trash)->tooltip("The next step that needs a password will ask for it again");

    // --- logs ---

    ttk::Box *logs = pane();

    logs->append(card(inside, 12.0));
    inside->append(Parts::section("LOGS"));

    ttk::Box *kept = inside->append(ttk::Box::row());
    kept->spacing(16.0)->cross(ttk::Box::Place::Centre);

    ttk::Box *about = kept->append(ttk::Box::column());
    about->spacing(3.0);
    about->stretch = 1.0;

    ttk::Box *aboutHead = about->append(ttk::Box::row());
    aboutHead->spacing(8.0)->cross(ttk::Box::Place::Centre);
    aboutHead->append(Parts::text("Kept for every run", 600, ttk::Theme::fontBody, palette().text));
    _logsSize = aboutHead->append(Parts::pill("none", palette().faint, palette().mutedSoft, false));
    aboutHead->append(std::make_unique<ttk::Spacer>());
    _logsNote = about->append(Parts::note(""));

    kept->append(std::make_unique<ttk::Button>("Open", [this] { _reach->logs.open(""); }))
        ->glyph(ttk::Glyphs::Glyph::Folder);
    _deleteAll = kept->append(std::make_unique<ttk::Button>("Delete all", [this] {
        _reach->ask("Delete every log?",
                    "Every run ever filed under " + Desk::pretty(Logs::directory())
                        + " is removed, for every kernel and for maintenance runs. Nothing else is touched.",
                    "Delete them all", true, [this] { _reach->logs.remove(""); });
    }));
    _deleteAll->glyph(ttk::Glyphs::Glyph::Trash)
        ->tooltip("Delete every log for every kernel, maintenance runs included");

    _logsRule = inside->append(Parts::above(2.0, std::make_unique<Parts::Rule>()));
    _noLogs = inside->append(Parts::note("Nothing has been filed yet. A run writes its log as it goes, and it "
                                         "stays here afterwards."));

    _holdingsList = inside->append(ttk::Box::column());
    _holdingsList->spacing(12.0);

    // --- the footer ---

    _footer = append(std::make_unique<ttk::Panel>());

    ttk::Box *footer = _footer->append(ttk::Box::row());
    footer->pad(12.0)->spacing(8.0)->cross(ttk::Box::Place::Centre);

    ttk::Label *unsaved = footer->append(Parts::note(
        "Unsaved changes. Directories that do not exist yet are created when you save."));
    unsaved->tone(palette().warning);
    unsaved->stretch = 1.0;

    footer->append(std::make_unique<ttk::Button>("Revert", [this] {
        _reach->settings.load();
        _reach->notify.info("Settings reloaded from disk.");
    }))->glyph(ttk::Glyphs::Glyph::Refresh)->tooltip("Throw away these changes and read the file again");

    footer->append(std::make_unique<ttk::Button>("Save", [this] { store(); }))
        ->glyph(ttk::Glyphs::Glyph::Check)->kind(ttk::Button::Kind::Primary);

    load();
}

void SettingsPage::arrange(ttk::Typeface &type) {
    const double rail = _box.w < 720.0 ? 132.0 : 168.0;
    const double left = _box.x + rail + RAIL_GAP;
    const double wide = std::max(0.0, _box.x + _box.w - left);
    const bool footer = _footer->visible();
    const double bottom = footer ? _box.y + _box.h - 2.0 - FOOTER_HEIGHT - 12.0 : _box.y + _box.h;

    _rail->place(BLRect{_box.x, _box.y, rail, _box.h}, type);

    for (ttk::Scroll *pane : _panes) {
        pane->place(BLRect{left, _box.y, wide, std::max(0.0, bottom - _box.y)}, type);
    }

    _footer->place(BLRect{left, _box.y + _box.h - 2.0 - FOOTER_HEIGHT, std::max(0.0, wide - 12.0), FOOTER_HEIGHT},
                   type);
}

// Empty means one job per core, which is what a zero is in the file.
int SettingsPage::wanted_jobs() const {
    const std::string typed = ttk::Text::trim(_jobs->text());

    return typed.empty() ? 0 : std::max(0, ttk::Text::to_int(typed));
}

bool SettingsPage::dirty() const {
    const SettingsBridge &settings = _reach->settings;

    return ttk::Text::trim(_base->text()) != settings.baseDirectory
        || ttk::Text::trim(_archives->text()) != settings.archiveDirectory
        || ttk::Text::trim(_boot->text()) != settings.bootDirectory
        || ttk::Text::trim(_grub->text()) != settings.grubDirectory
        || ttk::Text::trim(_cdn->text()) != settings.kernelCdn
        || ttk::Text::trim(_format->text()) != settings.archiveFormat
        || ttk::Text::trim(_elevation->text()) != settings.elevationCommand
        || wanted_jobs() != settings.jobs
        || _wantedCompiler != settings.compiler
        || ttk::Text::trim(_flags->text()) != settings.customFlags
        || _wantedNotifications != settings.notifications;
}

void SettingsPage::load() {
    const SettingsBridge &settings = _reach->settings;

    _base->set_text(settings.baseDirectory);
    _archives->set_text(settings.archiveDirectory);
    _boot->set_text(settings.bootDirectory);
    _grub->set_text(settings.grubDirectory);
    _cdn->set_text(settings.kernelCdn);
    _format->set_text(settings.archiveFormat);
    _jobs->set_text(settings.jobs > 0 ? std::to_string(settings.jobs) : std::string{});
    _elevation->set_text(settings.elevationCommand);
    _wantedCompiler = settings.compiler;
    _flags->set_text(settings.customFlags);
    _wantedNotifications = settings.notifications;
    _settingsSeen = settings.revision();
}

void SettingsPage::store() {
    if (!dirty()) {
        return;
    }

    SettingsBridge &settings = _reach->settings;

    settings.baseDirectory = ttk::Text::trim(_base->text());
    settings.archiveDirectory = ttk::Text::trim(_archives->text());
    settings.bootDirectory = ttk::Text::trim(_boot->text());
    settings.grubDirectory = ttk::Text::trim(_grub->text());
    settings.kernelCdn = ttk::Text::trim(_cdn->text());
    settings.archiveFormat = ttk::Text::trim(_format->text());
    settings.jobs = wanted_jobs();
    settings.elevationCommand = ttk::Text::trim(_elevation->text());
    settings.compiler = _wantedCompiler;
    settings.customFlags = ttk::Text::trim(_flags->text());
    settings.notifications = _wantedNotifications;
    settings.save();
}

// Not bound, so a run or a delete brings it up to date.
void SettingsPage::read_logs() {
    _logsRead = _reach->logs.revision();
    _logSize = Logs::size("");
    _holdings = Logs::all();
    sync_holdings();
}

void SettingsPage::sync_holdings() {
    _rows.clear();
    _stepsBoxes.clear();
    _steps.clear();
    _holdingsList->clear();

    for (const Log::Holding &holding : _holdings) {
        ttk::Box *group = _holdingsList->append(ttk::Box::column());
        group->spacing(6.0);

        const std::string name = holding.name;

        _rows.push_back(group->append(std::make_unique<HoldingRow>(_reach, holding, [this, name] {
            _openedLog = _openedLog == name ? std::string{} : name;
            _reach->touch();
        })));

        auto steps = std::make_unique<LogSteps>(_reach);
        _steps.push_back(steps.get());

        std::unique_ptr<ttk::Box> indented = ttk::Box::column();
        indented->pad(20.0, 0.0, 0.0, 4.0);
        indented->add(std::move(steps));
        _stepsBoxes.push_back(group->append(std::move(indented)));
    }

    if (root() != nullptr) {
        root()->relayout();
    }
}

void SettingsPage::sync() {
    if (_logsRead != _reach->logs.revision()) {
        read_logs();
    }

    const SettingsBridge &settings = _reach->settings;

    // The file was read again, by a save or a revert, so the form follows it.
    if (_settingsSeen != settings.revision()) {
        load();
    }
    const bool running = _reach->workflow.running();

    _rail->invalidate();

    for (size_t index = 0; index < _panes.size(); ++index) {
        _panes[index]->set_visible(std::cmp_equal(index, _section));
    }

    _notifications->set_checked(_wantedNotifications);

    // Follows the field rather than the saved value, since the page shows what Save would write.
    if (const std::vector<std::string> names = { "GCC", "LLVM", settings.customToolchainName.empty()
                                                                     ? std::string("Custom")
                                                                     : settings.customToolchainName };
        names != _toolchainNames) {
        _toolchainNames = names;
        _toolchain->set_options(names);
    }

    _toolchain->set_disabled(ttk::Text::trim(_flags->text()).empty() ? std::vector<int>{ 2 } : std::vector<int>{});
    _toolchain->set_current(static_cast<int>(
        std::ranges::find(TOOLCHAINS, _wantedCompiler) - std::begin(TOOLCHAINS)) % 3);

    _forget->set_enabled(SettingsBridge::password_remembered());

    _logsSize->set_text(_logSize.empty() ? "none" : _logSize);
    _logsNote->set_text("Every run writes one under " + Desk::pretty(Logs::directory())
                        + ". Nothing clears them up on its own, so they are listed below by what wrote them, "
                          "heaviest first.");
    _deleteAll->set_enabled(!running);
    _logsRule->set_visible(!_holdings.empty());
    _noLogs->set_visible(_holdings.empty());

    for (size_t index = 0; index < _rows.size(); ++index) {
        const bool open = _openedLog == _rows[index]->name();

        _rows[index]->set_open(open);
        _rows[index]->set_idle(!running);
        _stepsBoxes[index]->set_visible(open);
        _steps[index]->set_idle(!running);

        if (open && !_steps[index]->loaded()) {
            _steps[index]->set_steps(_rows[index]->name(), Logs::build_logs(_rows[index]->name()));
        }
    }

    if (const bool unsaved = dirty(); unsaved != _footer->visible()) {
        _footer->set_visible(unsaved);
    }
}
