// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <filesystem>

#include "core/Configuration.h"
#include "core/KernelArchive.h"
#include "gui/model/Catalog.h"
#include "gui/model/Format.h"

namespace {

    Catalog::Build describe(const Kernel &kernel, const std::string &running) {
        const Kernel::Version version = kernel.get_version();
        const Kernel::Status status = kernel.get_status();
        const std::string source = kernel.get_source_directory();
        const bool extracted = std::filesystem::is_directory(source);

        Catalog::Build build;

        build.name = version.get_string();
        build.extracted = extracted;
        build.version = version.get_string(Kernel::Version::INCLUDE_ZERO_REVISION);
        build.suffix = version.suffix;
        build.source = extracted ? source : std::string{};
        build.patched = kernel.is_patched();
        build.toolchain = toolchain_name(Kernel::built_with(version));
        build.configured = kernel.is_configured();
        build.built = kernel.is_built();
        build.modules = (status & Kernel::ModulesInstalled) != 0;
        build.image = (status & Kernel::ImageInstalled) != 0;
        build.initramfs = (status & Kernel::InitramsInstalled) != 0;
        build.installed = (status & (Kernel::ModulesInstalled | Kernel::ImageInstalled)) != 0;
        build.imagePath = kernel.get_image();
        build.signedImage = kernel.is_signed();
        build.running = build.name == running;

        return build;
    }
}

std::string Catalog::Entry::summary() const {
    std::vector<std::string> parts;

    if (archived) {
        parts.push_back("archive " + size);
    }

    if (!builds.empty()) {
        parts.push_back(std::to_string(builds.size()) + (builds.size() == 1 ? " build" : " builds"));
    }

    if (!installed.empty()) {
        parts.push_back(std::to_string(installed.size()) + " installed");
    }

    if (parts.empty()) {
        return "nothing local";
    }

    std::string joined;

    for (const std::string &part : parts) {
        joined += (joined.empty() ? "" : "  ·  ") + part;
    }

    return joined;
}

Catalog::Catalog(ttk::Clock *clock) : _watcher(clock, [this] { refresh(); }) {
    _running = Kernel::get_current().get_version().get_string();

    refresh();
}

int Catalog::archive_count() const {
    return static_cast<int>(std::ranges::count_if(_entries, [](const Entry &entry) { return entry.archived; }));
}

int Catalog::build_count() const {
    int builds = 0;

    for (const Entry &entry : _entries) {
        builds += static_cast<int>(entry.builds.size());
    }

    return builds;
}

void Catalog::refresh() {
    std::vector<Entry> entries;

    const auto find = [&entries](const Kernel::Version &version) -> Entry & {
        for (Entry &entry : entries) {
            if (entry.version == version) {
                return entry;
            }
        }

        Entry entry;
        entry.version = version;
        entry.name = version.get_string(Kernel::Version::INCLUDE_ZERO_REVISION);

        entries.push_back(std::move(entry));

        return entries.back();
    };

    for (const KernelArchive &archive : KernelArchive::list_archives()) {
        Entry &entry = find(archive.get_version());
        std::error_code error;
        const std::uintmax_t size = std::filesystem::file_size(archive.get_location(), error);

        entry.archived = true;
        entry.location = archive.get_location();
        entry.size = error ? std::string{} : Format::size(size);
    }

    for (const Kernel &kernel : Kernel::list_extracted()) {
        Kernel::Version version = kernel.get_version();
        version.suffix.clear();

        Entry &entry = find(version);
        const Build build = describe(kernel, _running);

        entry.builds.push_back(build);

        if (build.installed) {
            entry.installed.push_back(build);
        }

        if (build.running) {
            entry.running = true;
        }
    }

    // A kernel installed with no archive or build directory left still gets a
    // row, so it can be removed.
    for (const Kernel &kernel : Kernel::list_installed()) {
        Kernel::Version version = kernel.get_version();
        const std::string name = version.get_string();
        version.suffix.clear();

        Entry &entry = find(version);
        const bool listed = std::ranges::any_of(entry.installed, [&name](const Build &other) {
            return other.name == name;
        });

        if (listed) {
            continue;
        }

        const Build installed = describe(kernel, _running);

        entry.installed.push_back(installed);

        if (installed.running) {
            entry.running = true;
        }
    }

    std::ranges::sort(entries, [](const Entry &a, const Entry &b) { return a.version > b.version; });

    _entries = std::move(entries);
    _revision++;

    watch();

    if (changed) {
        changed();
    }
}

// A directory that has just been made has to be picked up as well.
void Catalog::watch() {
    const Settings *settings = Configuration::get();

    _watcher.watch({ settings->baseDirectory, settings->archiveDirectory });
}

const Catalog::Entry *Catalog::at(const int row) const {
    if (row < 0 || row >= static_cast<int>(_entries.size())) {
        return nullptr;
    }

    return &_entries[static_cast<size_t>(row)];
}

int Catalog::index_of_version(const std::string &version) const {
    for (size_t row = 0; row < _entries.size(); ++row) {
        if (_entries[row].name == version) {
            return static_cast<int>(row);
        }
    }

    return -1;
}

bool Catalog::remove_archive(const int row) {
    if (row < 0 || row >= static_cast<int>(_entries.size()) || !_entries[static_cast<size_t>(row)].archived) {
        return false;
    }

    std::error_code error;
    std::filesystem::remove(_entries[static_cast<size_t>(row)].location, error);
    refresh();

    return !error;
}

int Catalog::remove_archives() {
    int removed = 0;

    for (const Entry &entry : _entries) {
        if (!entry.archived) {
            continue;
        }

        std::error_code error;

        if (std::filesystem::remove(entry.location, error); !error) {
            removed++;
        }
    }

    refresh();

    return removed;
}
