// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_PAGES_KERNELSPAGE_H
#define KERNELMGR_GUI_PAGES_KERNELSPAGE_H


#include <string>
#include <vector>

#include "gui/components/BuildCard.h"
#include "gui/components/EmptyState.h"
#include "gui/components/InstalledCard.h"
#include "gui/components/SectionTabs.h"
#include "gui/components/VersionList.h"
#include "gui/model/Catalog.h"
#include "ttk/toolkit/controls/Button.h"
#include "ttk/toolkit/controls/Field.h"
#include "ttk/toolkit/controls/GlyphButton.h"
#include "ttk/toolkit/controls/Label.h"
#include "ttk/toolkit/controls/Pill.h"
#include "ttk/toolkit/controls/TextBox.h"
#include "ttk/toolkit/layout/Box.h"
#include "ttk/toolkit/layout/Panel.h"
#include "ttk/toolkit/layout/Scroll.h"
#include "ttk/toolkit/Widget.h"

struct Reach;

class KernelsPage : public ttk::Widget {
public:
    explicit KernelsPage(Reach *reach);

    void sync();

    // Show a version now if it is here, or as soon as it turns up.
    void show(const std::string &version);

    void arrange(ttk::Typeface &type) override;

private:
    enum class Section : std::uint8_t {
        Builds,
        Installed,
        Logs
    };

    class Adder : public ttk::Widget {
    public:
        explicit Adder(KernelsPage *page);

        [[nodiscard]] std::string typed() const;
        void clear() const;
        void sync() const;

        void arrange(ttk::Typeface &type) override;
        void paint(const ttk::Painter &painter) override;

    private:
        KernelsPage *_page;
        ttk::TextBox *_input = nullptr;
        ttk::GlyphButton *_get = nullptr;
    };

    [[nodiscard]] bool idle_for(const std::string &version) const;
    [[nodiscard]] const Catalog::Entry *entry() const;
    [[nodiscard]] bool entry_idle() const;

    void fetch();
    void reselect();
    void settle();
    void read_logs();
    void run(const std::string &name, const std::string &version, const std::string &suffix);
    void extract();
    void set_section(Section section);

    void sync_list();
    void sync_entry();
    void sync_builds(const Catalog::Entry &entry);
    void sync_installed(const Catalog::Entry &entry);
    void sync_logs();

    Reach *_reach;

    int _current = -1;
    // A version asked for, to select once it lands.
    std::string _wanted;
    // The build whose options are open, held here so it survives a re-read.
    std::string _opened;
    Section _section = Section::Builds;
    int _catalogSeen = -1;
    int _logsRead = -1;
    std::string _shownEntry;
    std::string _logSize;
    std::string _logHome;
    std::string _suffix;
    std::string _shape;

    ttk::Box *_head = nullptr;
    ttk::Label *_count = nullptr;
    ttk::Panel *_left = nullptr;
    Adder *_adder = nullptr;
    VersionList *_list = nullptr;
    EmptyState *_nothingOnDisk = nullptr;

    ttk::Panel *_right = nullptr;
    EmptyState *_nothingSelected = nullptr;
    ttk::Widget *_gap = nullptr;
    ttk::Box *_title = nullptr;
    ttk::Label *_name = nullptr;
    ttk::Pill *_running = nullptr;
    ttk::Panel *_archive = nullptr;
    ttk::Pill *_archiveSize = nullptr;
    ttk::Label *_archivePath = nullptr;
    ttk::Box *_extracting = nullptr;
    ttk::Field *_suffixField = nullptr;
    ttk::GlyphButton *_extract = nullptr;
    ttk::GlyphButton *_deleteArchive = nullptr;
    ttk::Box *_fetching = nullptr;
    ttk::Label *_fetchFrom = nullptr;
    ttk::GlyphButton *_download = nullptr;
    SectionTabs *_tabs = nullptr;
    ttk::Scroll *_body = nullptr;
    ttk::Box *_buildsSection = nullptr;
    EmptyState *_noBuilds = nullptr;
    ttk::Box *_builds = nullptr;
    std::vector<BuildCard *> _buildCards;
    ttk::Box *_installedSection = nullptr;
    EmptyState *_noInstalled = nullptr;
    ttk::Box *_installed = nullptr;
    std::vector<InstalledCard *> _installedCards;
    ttk::Box *_logsSection = nullptr;
    EmptyState *_noLogs = nullptr;
    ttk::Panel *_logs = nullptr;
    ttk::Pill *_logsSize = nullptr;
    ttk::Label *_logsPath = nullptr;
    ttk::Button *_deleteLogs = nullptr;
};


#endif //KERNELMGR_GUI_PAGES_KERNELSPAGE_H
