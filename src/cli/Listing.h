// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_LISTING_H
#define KERNELMGR_LISTING_H


#include <algorithm>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>
#include <vector>

#include "Ink.h"
#include "Reporter.h"
#include "core/Configuration.h"
#include "core/Kernel.h"
#include "core/KernelArchive.h"
#include "core/Log.h"

namespace Listing {
static std::string flag(const std::string &word, const bool held, const char *color = GREEN) {
    return Ink::paint(word, held ? color : DIM);
}

// Colour escapes take no room, so padding counts the plain length.
static std::string pad(const std::string &painted, const size_t plain, const size_t room) {
    return painted + std::string(plain < room ? room - plain : 0, ' ');
}

static std::string lead(const std::string &text, const size_t room) {
    return std::string(text.length() < room ? room - text.length() : 0, ' ') + text;
}

static void fact(const std::string &label, const std::string &value) {
    std::cout << Ink::paint(std::format("{:<8}", label), DIM) << "  " << value << "\n";
}

template <typename Iterator>
size_t widest(Iterator first, Iterator last) {
    size_t room = 10;

    for (; first != last; ++first) {
        room = std::max(room, first->get_version().get_string().length());
    }

    return room + 2;
}

static void heading(const std::string &title, const size_t count) {
    std::cout << "\n" << Ink::paint(title, BOLD)
        << "  " << Ink::paint(std::to_string(count), DIM) << "\n";
}

static void nothing(const std::string &what) {
    std::cout << "  " << Ink::paint(what, DIM) << "\n";
}

static std::string size_of(const std::string &path) {
    std::error_code error;
    const std::uintmax_t bytes = std::filesystem::file_size(path, error);

    return error ? "?" : Reporter::human_size(static_cast<double>(bytes));
}

inline int run() {
    const Settings *settings = Configuration::get();
    const std::string running = Kernel::get_current().get_version().get_string();

    const std::vector<KernelArchive> archives = KernelArchive::list_archives();
    const std::vector<Kernel> extracted = Kernel::list_extracted();
    const std::vector<Kernel> installed = Kernel::list_installed();

    fact("Running", Ink::paint(running, BOLD));
    fact("Base", Ink::pretty(settings->baseDirectory));
    fact("Archives", Ink::pretty(settings->archiveDirectory));

    if (const std::uintmax_t logs = Log::weigh(); logs > 0) {
        fact("Logs", Reporter::human_size(static_cast<double>(logs))
            + Ink::paint(" in " + Ink::pretty(Log::directory()), DIM));
    }

    heading("ARCHIVES", archives.size());

    if (archives.empty()) {
        nothing("Nothing downloaded.");
    }

    size_t column = widest(archives.begin(), archives.end());

    for (const KernelArchive &archive : archives) {
        const std::string name = archive.get_version().get_string();
        const std::string size = size_of(archive.get_location());

        std::cout << "  " << pad(Ink::paint(name, BOLD), name.length(), column)
            << lead(size, 9) << "   "
            << Ink::paint(archive.get_name(), DIM) << "\n";
    }

    heading("EXTRACTED", extracted.size());

    if (extracted.empty()) {
        nothing("Nothing unpacked.");
    }

    column = widest(extracted.begin(), extracted.end());

    for (const Kernel &kernel : extracted) {
        const bool patched = kernel.is_patched();
        const bool configured = kernel.is_configured();
        const bool built = kernel.is_built();
        const bool here = (kernel.get_status()
            & (Kernel::ModulesInstalled | Kernel::ImageInstalled)) != 0;

        const std::string name = kernel.get_version().get_string();
        const std::string first = configured ? "configured" : "not configured";
        const std::string second = built ? "built" : "not built";
        const std::string third = here ? "installed" : "not installed";

        // Unpatched is the ordinary case, so only a patched tree is marked.
        const std::string mark = patched ? "patched" : "";

        std::cout << "  " << pad(Ink::paint(name, BOLD), name.length(), column)
            << pad(flag(mark, patched), mark.length(), 9)
            << pad(flag(first, configured), first.length(), 16)
            << pad(flag(second, built), second.length(), 11)
            << flag(third, here) << "\n";
    }

    heading("INSTALLED", installed.size());

    if (installed.empty()) {
        nothing("Nothing installed that this can see.");
    }

    column = widest(installed.begin(), installed.end());

    for (const Kernel &kernel : installed) {
        const Kernel::Status status = kernel.get_status();
        const bool endorsed = kernel.is_signed();

        const std::string name = kernel.get_version().get_string();
        const std::string mark = endorsed ? "signed" : "unsigned";
        const bool now = name == running;

        std::cout << "  " << pad(Ink::paint(name, BOLD), name.length(), column)
            << pad(flag("modules", (status & Kernel::ModulesInstalled) != 0), 7, 9)
            << pad(flag("image", (status & Kernel::ImageInstalled) != 0), 5, 7)
            << pad(flag("initramfs", (status & Kernel::InitramsInstalled) != 0), 9, 11)
            << (now ? pad(flag(mark, endorsed), mark.length(), 10) : flag(mark, endorsed))
            << (now ? Ink::paint("running now", GREEN) : "") << "\n";
    }

    std::cout << "\n";

    return 0;
}
}


#endif //KERNELMGR_LISTING_H
