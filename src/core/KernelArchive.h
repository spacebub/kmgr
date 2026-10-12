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


#include "Kernel.h"
#include "Progress.h"
#include <string>
#include <vector>

class KernelArchive {
    Kernel::Version _kernelVersion;
    std::string _path;
    std::string _url;

    static std::string get_path(const Kernel::Version &version);
    static std::string get_url(const Kernel::Version &version);

public:
    explicit KernelArchive(const Kernel::Version &kernelVersion);
    explicit KernelArchive(const std::string &kernelVersion);
    KernelArchive(const std::string &name, const std::string &path);

    [[nodiscard]] std::string get_url() const;
    [[nodiscard]] Kernel::Version get_version() const;
    [[nodiscard]] std::string get_location() const;
    [[nodiscard]] std::string get_name(bool extension = true) const;

    // The signed list of digests kernel.org keeps beside the archives of a major version.
    [[nodiscard]] std::string get_checksum_url() const;

    // The digest the list gives for a file, or empty when the file is not in it.
    static std::string find_checksum(const std::string &listing, const std::string &name);

    void with_suffix(const std::string &suffix);
    void extract(Progress *progress = nullptr) const;
    void remove() const;

    static std::vector<KernelArchive> list_archives();
};


#endif //KERNELMGR_ARCHIVE_H
