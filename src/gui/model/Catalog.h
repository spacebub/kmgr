// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_MODEL_CATALOG_H
#define KERNELMGR_GUI_MODEL_CATALOG_H


#include <functional>
#include <string>
#include <vector>

#include "core/Kernel.h"
#include "gui/services/Watcher.h"

class Catalog {
public:
    struct Build {
        std::string name;
        std::string version;
        std::string suffix;
        std::string source;
        std::string toolchain;
        std::string imagePath;
        bool extracted = false;
        bool patched = false;
        bool configured = false;
        bool built = false;
        bool modules = false;
        bool image = false;
        bool initramfs = false;
        bool installed = false;
        bool signedImage = false;
        bool running = false;
    };

    struct Entry {
        Kernel::Version version{};
        // Written with a zero revision, which is how the pages name it.
        std::string name;
        bool archived = false;
        std::string location;
        std::string size;
        std::vector<Build> builds;
        std::vector<Build> installed;
        bool running = false;

        [[nodiscard]] std::string summary() const;
    };

    explicit Catalog(ttk::Clock *clock);

    [[nodiscard]] const std::vector<Entry> &entries() const { return _entries; }
    [[nodiscard]] int count() const { return static_cast<int>(_entries.size()); }
    [[nodiscard]] int revision() const { return _revision; }
    [[nodiscard]] int archive_count() const;
    [[nodiscard]] int build_count() const;

    void refresh();

    [[nodiscard]] const Entry *at(int row) const;
    [[nodiscard]] int index_of_version(const std::string &version) const;

    bool remove_archive(int row);
    int remove_archives();

    std::function<void()> changed;

private:
    void watch();

    std::vector<Entry> _entries;
    std::string _running;
    int _revision = 0;

    Watcher _watcher;
};


#endif //KERNELMGR_GUI_MODEL_CATALOG_H
