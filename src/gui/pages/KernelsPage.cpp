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
#include "gui/components/Parts.h"
#include "gui/pages/KernelsPage.h"
#include "ttk/draw/Theme.h"
#include "ttk/draw/Typeface.h"
#include "ttk/system/Text.h"
#include "ttk/toolkit/Root.h"
#include "ttk/toolkit/layout/Spacer.h"

namespace {
    using Palette = ttk::Theme::Palette;

    constexpr double GAP = 14.0;
    constexpr double LIST_WIDTH = 322.0;
    constexpr double LIST_HEIGHT = 220.0;

    const ttk::Theme::Palette &palette() {
        return ttk::Theme::palette();
    }
}

KernelsPage::Adder::Adder(KernelsPage *page) : _page(page) {
    _input = append(std::make_unique<ttk::TextBox>([this](const std::string &) { sync(); }));
    _input->mono();
    _input->accepted = [this] { _page->fetch(); };

    _get = append(Parts::glyph_button(ttk::Glyphs::Glyph::Download, 30.0,
                                      "Download this version from kernel.org", &Palette::accent,
                                      &Palette::accent, &Palette::accentSoft, [this] { _page->fetch(); }));
    _get->set_visible(false);
}

std::string KernelsPage::Adder::typed() const {
    return ttk::Text::trim(_input->text());
}

void KernelsPage::Adder::clear() const {
    _input->set_text("");
    sync();
}

void KernelsPage::Adder::sync() const {
    const SystemStatus &system = _page->_reach->system;

    _input->placeholder(system.latest_known() ? "Add version, e.g. " + system.latest() : "Version to download");
    _get->set_visible(!_input->text().empty());
    _get->tone(_page->idle_for(typed()) ? &Palette::accent : &Palette::faint, &Palette::accent);
    invalidate();
}

void KernelsPage::Adder::arrange(ttk::Typeface &type) {
    const double middle = _box.y + (_box.h / 2.0);
    const double left = _box.x + 3.0 + 30.0;
    const double right = _box.x + _box.w - 3.0 - 30.0 - 6.0;

    _input->place(BLRect{left, middle - 12.0, std::max(0.0, right - left), 24.0}, type);
    _get->place(BLRect{_box.x + _box.w - 3.0 - 30.0, middle - 15.0, 30.0, 30.0}, type);
}

void KernelsPage::Adder::paint(const ttk::Painter &painter) {
    const bool focused = _input->focused();

    if (focused) {
        painter.round(_box, ttk::Theme::radiusSmall, palette().field);
        painter.outline(_box, ttk::Theme::radiusSmall, 1.0, palette().accent);
    }

    // A mark, not a button: the row itself is the control.
    const BLRgba32 ink = !_input->text().empty() || focused ? palette().accent : palette().faint;
    const double cx = _box.x + 3.0 + 15.0;
    const double cy = _box.y + (_box.h / 2.0);

    painter.round(BLRect{cx - 6.5, cy - 0.9, 13.0, 1.8}, 1.0, ink);
    painter.round(BLRect{cx - 0.9, cy - 6.5, 1.8, 13.0}, 1.0, ink);

    Widget::paint(painter);
}

KernelsPage::KernelsPage(Reach *reach) : _reach(reach) {
    _head = append(ttk::Box::row());
    _head->spacing(10.0)->cross(ttk::Box::Place::Centre);
    _head->append(Parts::text("Versions on this machine", 600, ttk::Theme::fontMedium, &Palette::text));
    _head->append(std::make_unique<ttk::Spacer>());
    _count = _head->append(Parts::text("", 400, ttk::Theme::fontSmall, &Palette::faint));
    _head->append(Parts::glyph_button(ttk::Glyphs::Glyph::Refresh, ttk::Theme::controlSmall,
                                      "Read the base directory again", &Palette::muted, &Palette::text,
                                      &Palette::hover, [this] {
        _reach->refresh();
        _reach->notify.info("Reloaded from " + SystemStatus::base_directory() + ".");
    }));

    // --- the list ---

    _left = append(std::make_unique<ttk::Panel>());

    ttk::Box *listing = _left->append(ttk::Box::column());
    listing->pad(12.0)->spacing(8.0);

    _adder = listing->append(std::make_unique<Adder>(this));
    _adder->fixedHeight = ttk::Theme::control;

    listing->append(std::make_unique<Parts::Rule>(0.7));

    _list = listing->append(std::make_unique<VersionList>(false, [this](const int row) {
        _current = row;
        _reach->touch();
    }));
    _list->stretch = 1.0;

    _nothingOnDisk = listing->append(std::make_unique<EmptyState>(
        "Nothing on disk yet", "Type a version above to fetch it from kernel.org."));

    // --- the version ---

    _right = append(std::make_unique<ttk::Panel>());

    ttk::Box *detail = _right->append(ttk::Box::column());
    detail->pad(18.0)->spacing(12.0);

    _nothingSelected = detail->append(std::make_unique<EmptyState>(
        "Nothing selected", "Pick a version on the left, or download one to start with."));
    _gap = detail->append(std::make_unique<ttk::Spacer>());

    _title = detail->append(ttk::Box::row());
    _title->spacing(10.0)->cross(ttk::Box::Place::Centre);
    _name = _title->append(Parts::text("", palette().headingWeight, ttk::Theme::fontDisplay, &Palette::text));
    _name->stretch = 1.0;
    _running = _title->append(Parts::pill("running now", &Palette::success, &Palette::successSoft));

    _archive = detail->append(std::make_unique<ttk::Panel>());
    _archive->inset = true;

    ttk::Box *archive = _archive->append(ttk::Box::column());
    archive->pad(16.0)->spacing(10.0);

    ttk::Box *archiveHead = archive->append(ttk::Box::row());
    archiveHead->cross(ttk::Box::Place::Centre);
    archiveHead->append(Parts::section("ARCHIVE"))->stretch = 1.0;
    _archiveSize = archiveHead->append(Parts::pill("", &Palette::accent, &Palette::accentSoft));

    // The tarball is named, but opening it opens where it sits.
    _archivePath = archive->append(Parts::path(reach, "", true, SystemStatus::archive_directory()));

    _extracting = archive->append(ttk::Box::row());
    _extracting->spacing(8.0)->cross(ttk::Box::Place::Centre);

    _suffixField = _extracting->append(std::make_unique<ttk::Field>("Extract as", [this](const std::string &text) {
        // The field draws the dash before the suffix, so a typed one is stripped as it is typed.
        const size_t dashes = text.find_first_not_of('-');

        _suffix = dashes == std::string::npos ? std::string{} : text.substr(dashes);

        if (_suffix != text) {
            _suffixField->set_text(_suffix);
        }

        _reach->touch();
    }));
    _suffixField->placeholder("suffix, optional")->mono();
    _suffixField->stretch = 1.0;
    _suffixField->icon(ttk::Glyphs::Glyph::Down, "", [this] {
        _suffix = _reach->system.suffix();
        _suffixField->set_text(_suffix);
        _reach->touch();
    });

    _extract = _extracting->append(Parts::glyph_button(ttk::Glyphs::Glyph::Extract, 34.0,
                                                       "Unpack the archive under the suffix on the left",
                                                       &Palette::accent, &Palette::accent, &Palette::accentSoft,
                                                       [this] { extract(); }));
    _extract->outlined();

    _deleteArchive = _extracting->append(Parts::glyph_button(
        ttk::Glyphs::Glyph::Trash, 34.0, "Delete the tarball. Anything extracted from it stays", &Palette::faint,
        &Palette::danger, &Palette::dangerSoft, [this] {
            const Catalog::Entry *held = entry();

            if (held == nullptr) {
                return;
            }

            _reach->ask("Delete the " + held->name + " archive?",
                        "The tarball is removed from disk. Anything already extracted from it stays.",
                        "Delete it", true, [this] {
                            _reach->catalog.remove_archive(_current);
                            _reach->notify.success("Archive deleted.");
                        });
        }));
    _deleteArchive->outlined();

    _fetching = archive->append(ttk::Box::row());
    _fetching->spacing(10.0)->cross(ttk::Box::Place::Centre);
    _fetchFrom = _fetching->append(Parts::text("", 400, ttk::Theme::fontSmall, &Palette::faint));
    _fetchFrom->stretch = 1.0;
    _download = _fetching->append(Parts::glyph_button(ttk::Glyphs::Glyph::Download, 34.0, "", &Palette::accent,
                                                      &Palette::accent, &Palette::accentSoft, [this] {
        if (const Catalog::Entry *held = entry(); held != nullptr && entry_idle()) {
            _reach->workflow.download(held->name);
        }
    }));
    _download->outlined();

    auto tabs = std::make_unique<SectionTabs>([this](const int index) {
        set_section(static_cast<Section>(index));
    });
    _tabs = tabs.get();
    _tabs->set_tabs({ { "Builds", "" }, { "Installed", "" }, { "Logs", "" } });
    detail->append(Parts::above(2.0, std::move(tabs)));

    _body = detail->append(std::make_unique<ttk::Scroll>());
    _body->stretch = 1.0;
    _body->minHeight = 90.0;

    ttk::Box *sections = static_cast<ttk::Box *>(_body->hold(ttk::Box::column()));
    sections->pad(0.0, 0.0, 10.0, 0.0)->spacing(12.0);

    _buildsSection = sections->append(ttk::Box::column());
    _buildsSection->spacing(12.0);

    auto noBuilds = std::make_unique<EmptyState>("Nothing extracted from this version", "");
    _noBuilds = noBuilds.get();
    _buildsSection->append(Parts::above(2.0, std::move(noBuilds)));
    _builds = _buildsSection->append(ttk::Box::column());
    _builds->spacing(12.0);

    _installedSection = sections->append(ttk::Box::column());
    _installedSection->spacing(12.0);

    auto noInstalled = std::make_unique<EmptyState>("Nothing installed from this version", "");
    _noInstalled = noInstalled.get();
    _installedSection->append(Parts::above(2.0, std::move(noInstalled)));
    _installed = _installedSection->append(ttk::Box::column());
    _installed->spacing(12.0);

    _logsSection = sections->append(ttk::Box::column());
    _logsSection->spacing(12.0);

    auto noLogs = std::make_unique<EmptyState>(
        "Nothing written yet", "Every step of every run files what it printed, and what it filed for this "
                               "version is kept here.");
    _noLogs = noLogs.get();
    _logsSection->append(Parts::above(2.0, std::move(noLogs)));

    auto logs = std::make_unique<ttk::Panel>();
    _logs = logs.get();
    _logs->inset = true;
    _logsSection->append(Parts::above(2.0, std::move(logs)));

    ttk::Box *filed = _logs->append(ttk::Box::column());
    filed->pad(16.0)->spacing(10.0);

    ttk::Box *filedHead = filed->append(ttk::Box::row());
    filedHead->cross(ttk::Box::Place::Centre);
    filedHead->append(Parts::section("ON DISK"))->stretch = 1.0;
    _logsSize = filedHead->append(Parts::pill("none", &Palette::faint, &Palette::mutedSoft, false));

    _logsPath = filed->append(Parts::path(reach, "", true));

    ttk::Box *actions = filed->append(ttk::Box::row());
    actions->spacing(8.0)->cross(ttk::Box::Place::Centre);
    actions->append(std::make_unique<ttk::Button>("Open", [this] {
        _reach->logs.open(entry() != nullptr ? entry()->name : std::string{});
    }))->glyph(ttk::Glyphs::Glyph::Folder)->compact()->tooltip("Open the logs in your file manager");

    _deleteLogs = actions->append(std::make_unique<ttk::Button>("Delete", [this] {
        const Catalog::Entry *held = entry();

        if (held == nullptr) {
            return;
        }

        const std::string version = held->name;

        _reach->ask("Delete the logs for " + version + "?",
                    "Every run filed under " + version + ", for all of its suffixes, is removed from "
                        + Desk::pretty(Logs::directory()) + ". Nothing else is touched.",
                    "Delete them", true, [this, version] { _reach->logs.remove(version); });
    }));
    _deleteLogs->glyph(ttk::Glyphs::Glyph::Trash)->kind(ttk::Button::Kind::Danger)->compact();
    actions->append(std::make_unique<ttk::Spacer>());

    sections->append(std::make_unique<ttk::Spacer>(0.0))->fixedHeight = 2.0;

    reselect();
}

void KernelsPage::arrange(ttk::Typeface &type) {
    const bool wide = _box.w > 720.0;
    const double head = _head->natural_height(type, _box.w);

    _head->place(BLRect{_box.x, _box.y, _box.w, head}, type);

    const double top = _box.y + head + GAP;
    const double tall = std::max(0.0, _box.y + _box.h - top);

    if (wide) {
        _left->place(BLRect{_box.x, top, LIST_WIDTH, tall}, type);
        _right->place(BLRect{_box.x + LIST_WIDTH + GAP, top, std::max(0.0, _box.w - LIST_WIDTH - GAP), tall},
                      type);

        return;
    }

    _left->place(BLRect{_box.x, top, _box.w, LIST_HEIGHT}, type);
    _right->place(BLRect{_box.x, top + LIST_HEIGHT + GAP, _box.w, std::max(0.0, tall - LIST_HEIGHT - GAP)},
                  type);
}

bool KernelsPage::idle_for(const std::string &version) const {
    const std::string busy = _reach->workflow.running() ? _reach->workflow.version() : std::string{};

    return busy.empty() || busy != version;
}

const Catalog::Entry *KernelsPage::entry() const {
    return _reach->catalog.at(_current);
}

bool KernelsPage::entry_idle() const {
    const Catalog::Entry *held = entry();

    return idle_for(held != nullptr ? held->name : std::string{});
}

void KernelsPage::show(const std::string &version) {
    if (version.empty()) {
        return;
    }

    if (const int row = _reach->catalog.index_of_version(version); row >= 0) {
        _current = row;
        _wanted.clear();
    } else {
        _wanted = version;
    }

    _reach->touch();
}

void KernelsPage::fetch() {
    const std::string wanted = _adder->typed();

    if (wanted.empty() || !idle_for(wanted)) {
        return;
    }

    _wanted = wanted;
    _adder->clear();
    _reach->workflow.download(wanted);
}

void KernelsPage::reselect() {
    const Catalog &catalog = _reach->catalog;

    if (!_wanted.empty()) {
        if (const int landed = catalog.index_of_version(_wanted); landed >= 0) {
            _current = landed;
            _wanted.clear();

            return;
        }
    }

    if (catalog.count() == 0) {
        _current = -1;
    } else if (_current < 0 || _current >= catalog.count()) {
        _current = std::max(0, std::min(_current, catalog.count() - 1));
    }
}

void KernelsPage::settle() {
    const Catalog::Entry *held = entry();

    if (held == nullptr) {
        return;
    }

    if (_section == Section::Builds && held->builds.empty() && !held->installed.empty()) {
        set_section(Section::Installed);
    } else if (_section == Section::Installed && held->installed.empty()) {
        set_section(Section::Builds);
    }
}

// A section is read from its top, and the one before it was left somewhere else.
void KernelsPage::set_section(const Section section) {
    if (_section == section) {
        return;
    }

    _section = section;
    _body->scroll_to(0.0);
    _reach->touch();
}

// Not bound, so it is re-read whenever a run or a delete changes it.
void KernelsPage::read_logs() {
    const Catalog::Entry *held = entry();

    _logsRead = _reach->logs.revision();
    _logSize = held != nullptr ? Logs::size(held->name) : std::string{};
    _logHome = Logs::directory();
}

void KernelsPage::run(const std::string &name, const std::string &version, const std::string &suffix) {
    WorkflowRunner &workflow = _reach->workflow;
    const std::string named = version + (suffix.empty() ? "" : "-" + suffix);
    const Catalog::Entry *held = entry();
    const bool built = held != nullptr && std::ranges::any_of(held->builds, [&named](const Catalog::Build &build) {
        return build.name == named && build.built;
    });

    if (name == "upToBuild") {
        workflow.up_to_build(version, suffix);
    } else if (name == "everything") {
        // It ends in an install, so it is asked for the same way one is.
        _reach->ask("Run everything for " + named + "?",
                    std::string(built ? "It is already built, so it picks up at installing it. Use the steps "
                                        "below to build it over."
                                      : "It is patched, configured, built and installed in one go, without "
                                        "stopping between the steps.")
                        + " The steps that write outside your home ask for your password.",
                    "Run everything", false, [this, version, suffix] { _reach->workflow.everything(version, suffix); });
    } else if (name == "patch") {
        workflow.patch(version, suffix);
    } else if (name == "revert") {
        workflow.revert(version, suffix);
    } else if (name == "configure") {
        workflow.configure(version, suffix);
    } else if (name == "build") {
        workflow.build(version, suffix);
    } else if (name == "install") {
        _reach->ask("Install " + named + "?",
                    "Modules, image and initramfs are installed and grub is refreshed. These steps need your "
                    "password.",
                    "Install", false, [this, version, suffix] { _reach->workflow.install(version, suffix); });
    } else if (name == "sources") {
        _reach->ask("Delete the sources of " + named + "?",
                    "Only the build directory goes. Anything already installed from it stays where it is, and "
                    "the archive is left alone.",
                    "Delete sources", true,
                    [this, version, suffix] { _reach->workflow.remove_sources(version, suffix); });
    }
}

void KernelsPage::extract() {
    const Catalog::Entry *held = entry();

    if (held == nullptr || !entry_idle()) {
        return;
    }

    const std::string version = held->name;
    const std::string suffix = _suffix;
    const std::string name = version + (suffix.empty() ? "" : "-" + suffix);
    const bool exists = std::ranges::any_of(held->builds, [&name](const Catalog::Build &build) {
        return build.name == name;
    });

    // What it makes is a build, so it is where the eye goes next.
    set_section(Section::Builds);

    if (!exists) {
        _reach->workflow.extract(version, suffix);

        return;
    }

    _reach->ask("Replace linux-" + name + "?",
                "That directory already exists. Extracting replaces it, and anything configured or built in "
                "it is lost.",
                "Replace it", true, [this, version, suffix] { _reach->workflow.extract(version, suffix); });
}

void KernelsPage::sync() {
    const Catalog &catalog = _reach->catalog;

    if (_catalogSeen != catalog.revision()) {
        _catalogSeen = catalog.revision();
        reselect();
        sync_list();
    }

    const Catalog::Entry *held = entry();
    const std::string shown = held != nullptr ? held->name : std::string{};

    if (shown != _shownEntry) {
        _shownEntry = shown;
        _body->scroll_to(0.0);
        _logsRead = -1;
    }

    if (_logsRead != _reach->logs.revision()) {
        read_logs();
    }

    settle();

    // A change in any of these moves its neighbours, so the page is laid out again.
    if (const std::string shape = std::to_string(catalog.count()) + '\n' + shown + '\n' + _logSize + '\n' + _suffix
            + '\n' + std::to_string(catalog.revision()) + '\n' + std::to_string(_reach->logs.revision())
            + (_reach->workflow.running() ? "r" : "");
        shape != _shape) {
        _shape = shape;

        if (root() != nullptr) {
            root()->relayout();
        }
    }

    _count->set_text(std::to_string(catalog.count()) + (catalog.count() == 1 ? " version" : " versions")
                     + " on disk");
    _adder->sync();
    _list->set_current(_current);
    _nothingOnDisk->set_visible(catalog.count() == 0);

    sync_entry();
}

void KernelsPage::sync_list() {
    std::vector<VersionList::Row> rows;

    for (const Catalog::Entry &entry : _reach->catalog.entries()) {
        rows.push_back(VersionList::Row{
            .version = entry.name,
            .summary = entry.summary(),
            .running = entry.running,
            .installed = static_cast<int>(entry.installed.size())
        });
    }

    _list->set_rows(std::move(rows));
}

void KernelsPage::sync_entry() {
    const Catalog::Entry *held = entry();
    const bool selected = held != nullptr;

    _nothingSelected->set_visible(!selected);
    _gap->set_visible(!selected);
    _title->set_visible(selected);
    _archive->set_visible(selected);
    _tabs->set_visible(selected);
    _body->set_visible(selected);

    if (!selected) {
        return;
    }

    const SystemStatus &system = _reach->system;
    const bool idle = entry_idle();

    _name->set_text("Linux " + held->name);
    _running->set_visible(held->running);

    _archiveSize->set_text(held->archived ? held->size : "not downloaded");
    const Parts::Tones tones = Parts::lit(held->archived, &Palette::accent, &Palette::accentSoft);
    _archiveSize->tones(tones.tone, tones.wash);
    _archiveSize->invalidate();

    _archivePath->set_visible(held->archived);
    _archivePath->set_text(held->location);

    _extracting->set_visible(held->archived);
    _suffixField->prefix("linux-" + held->name + (_suffix.empty() ? "" : "-"));
    _suffixField->set_icon_visible(!system.suffix().empty() && _suffix != system.suffix());
    _suffixField->note("Left empty it is unpacked as linux-" + held->name);
    _extract->tone(idle ? &Palette::accent : &Palette::faint, &Palette::accent);
    _extract->invalidate();

    _suffixField->set_icon_hint("Use " + system.suffix() + ", what the running kernel was built under");

    _fetching->set_visible(!held->archived);

    std::string cdn = _reach->settings.kernelCdn;

    if (cdn.starts_with("https://")) {
        cdn.erase(0, 8);
    }

    _fetchFrom->set_text("Fetch it from " + cdn);
    _download->tooltip("Download the " + held->name + " archive from kernel.org");
    _download->tone(idle ? &Palette::accent : &Palette::faint, &Palette::accent);
    _download->invalidate();

    _tabs->set_note(0, held->builds.empty() ? "" : std::to_string(held->builds.size()));
    _tabs->set_note(1, held->installed.empty() ? "" : std::to_string(held->installed.size()));
    _tabs->set_note(2, _logSize);
    _tabs->set_current(static_cast<int>(_section));

    _buildsSection->set_visible(_section == Section::Builds);
    _installedSection->set_visible(_section == Section::Installed);
    _logsSection->set_visible(_section == Section::Logs);

    sync_builds(*held);
    sync_installed(*held);
    sync_logs();
}

void KernelsPage::sync_builds(const Catalog::Entry &shown) {
    _noBuilds->set_visible(shown.builds.empty());
    _noBuilds->set_body(shown.archived ? "Extract the archive above to get a build directory to work in."
                                       : "Download the archive first, then extract it.");

    bool same = _buildCards.size() == shown.builds.size();

    for (size_t index = 0; same && index < shown.builds.size(); ++index) {
        same = _buildCards[index]->name() == shown.builds[index].name;
    }

    if (!same) {
        _buildCards.clear();
        _builds->clear();

        for (const Catalog::Build &build : shown.builds) {
            const std::string name = build.name;
            const std::string version = build.version;
            const std::string suffix = build.suffix;

            _buildCards.push_back(_builds->append(std::make_unique<BuildCard>(
                _reach, build, [this, name, version, suffix](const std::string &action) {
                    if (action == "options") {
                        _opened = _opened == name ? std::string{} : name;
                        _reach->touch();
                    } else {
                        run(action, version, suffix);
                    }
                })));
        }

        if (root() != nullptr) {
            root()->relayout();
        }
    }

    for (size_t index = 0; index < shown.builds.size(); ++index) {
        const Catalog::Build &build = shown.builds[index];

        _buildCards[index]->sync(build, idle_for(build.version), _reach->system.secure_boot(), _opened == build.name);
    }
}

void KernelsPage::sync_installed(const Catalog::Entry &shown) {
    _noInstalled->set_visible(shown.installed.empty());
    _noInstalled->set_body(shown.builds.empty() ? "Extract the archive and build it first."
                                                : "Install one of the builds and it turns up here.");

    bool same = _installedCards.size() == shown.installed.size();

    for (size_t index = 0; same && index < shown.installed.size(); ++index) {
        same = _installedCards[index]->name() == shown.installed[index].name;
    }

    if (!same) {
        _installedCards.clear();
        _installed->clear();

        for (const Catalog::Build &kernel : shown.installed) {
            const Catalog::Build copy = kernel;

            _installedCards.push_back(_installed->append(std::make_unique<InstalledCard>(
                kernel, [this, copy](const std::string &action) {
                    const Catalog::Entry *held = entry();

                    if (action == "download") {
                        if (held != nullptr) {
                            _reach->workflow.download(held->name);
                        }
                    } else if (action == "sign") {
                        _reach->workflow.sign(copy.version, copy.suffix);
                    } else {
                        _reach->ask("Remove " + copy.name + "?",
                                    std::string("Its modules, boot files and mkinitcpio preset are deleted.")
                                        + (copy.extracted ? " The build directory it came from stays, so it "
                                                            "can be installed again without building it over."
                                                          : "")
                                        + (copy.running ? " This is the kernel you are running right now." : ""),
                                    "Remove it", true, [this, copy] {
                                        _reach->workflow.remove_installed(copy.version, copy.suffix);
                                    });
                    }
                })));
        }

        if (root() != nullptr) {
            root()->relayout();
        }
    }

    for (size_t index = 0; index < shown.installed.size(); ++index) {
        const Catalog::Build &kernel = shown.installed[index];

        _installedCards[index]->sync(kernel, idle_for(kernel.version), _reach->system.secure_boot(),
                                     shown.archived, shown.name);
    }
}

void KernelsPage::sync_logs() {
    _noLogs->set_visible(_logSize.empty());
    _logs->set_visible(!_logSize.empty());
    _logsSize->set_text(_logSize.empty() ? "none" : _logSize);
    _logsPath->set_text(_logHome);

    const Catalog::Entry *held = entry();

    _deleteLogs->tooltip("Delete every log written for " + (held != nullptr ? held->name : "this version"));
    _deleteLogs->set_enabled(entry_idle());
}
