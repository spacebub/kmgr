// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2024
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_ARCHIVE_H
#define KERNELMGR_ARCHIVE_H


#include <string>
#include <vector>
#include "Kernel.h"
#include "Progress.h"

class KernelArchive {
    Kernel::Version _kernelVersion;
    std::string _path;
    std::string _url;
    int _fileCount = -1;

    static std::string get_path(const Kernel::Version &version);
    static std::string get_url(const Kernel::Version &version);

public:
    explicit KernelArchive(const Kernel::Version &kernelVersion);
    explicit KernelArchive(const std::string &kernelVersion);
    KernelArchive(const std::string &name, const std::string &path);

    std::string get_url();
    [[nodiscard]] Kernel::Version get_version() const;
    [[nodiscard]] std::string get_location() const;
    [[nodiscard]] std::string get_name(bool extension = true) const;
    int get_file_count();

    void with_suffix(const std::string &suffix);
    void extract(Progress *progress = nullptr) const;
    void remove() const;

    static std::vector<KernelArchive> list_archives();
};


#endif //KERNELMGR_ARCHIVE_H
