// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2024
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <filesystem>
#include <format>
#include "KernelArchive.h"
#include "Configuration.h"
#include <stdexcept>
#include "archive.h"
#include "archive_entry.h"

static int copy_data(archive *read, archive *write);

KernelArchive::KernelArchive(const Kernel::Version &kernelVersion) {
    _kernelVersion = kernelVersion;
    _path = get_path(kernelVersion);
    _url = get_url(kernelVersion);
}

KernelArchive::KernelArchive(const std::string &kernelVersion) {
    _kernelVersion = Kernel::get_version(kernelVersion);
    _path = get_path(_kernelVersion);
    _url = get_url(_kernelVersion);
}

KernelArchive::KernelArchive(const std::string &name, const std::string &path) {
    _kernelVersion = Kernel::get_version(name.substr(6));
    _path = path;
    _url = get_url(_kernelVersion);
}

std::string KernelArchive::get_url() {
    return _url;
}

Kernel::Version KernelArchive::get_version() const {
    return _kernelVersion;
}

std::string KernelArchive::get_location() const {
    return _path;
}

std::string KernelArchive::get_name(const bool extension) const {
    std::string name = "linux-" + _kernelVersion.get_string(Kernel::Version::NO_OPTIONS);

    if (extension) {
        name += "." + Configuration::get()->archiveFormat;
    }

    return name;
}

int KernelArchive::get_file_count() {
    if (_fileCount > 0) {
        return _fileCount;
    }

    if (!std::filesystem::exists(_path)) {
        throw std::runtime_error("You must download the archive first!");
    }

    archive *archive = archive_read_new();
    archive_entry *entry;
    _fileCount = 0;

    archive_read_support_filter_all(archive);
    archive_read_support_format_all(archive);

    if (archive_read_open_filename(archive, _path.c_str(), 10240)) {
        throw std::runtime_error(archive_error_string(archive));
    }

    for (;;) {
        const int status = archive_read_next_header(archive, &entry);\

        if (status == ARCHIVE_EOF) {
            break;
        }
        if (status != ARCHIVE_OK) {
            throw std::runtime_error(archive_error_string(archive));
        }

        archive_read_data_skip(archive);
        _fileCount++;
    }

    archive_read_close(archive);
    archive_read_free(archive);

    return _fileCount;
}

void KernelArchive::with_suffix(const std::string &suffix) {
    _kernelVersion.suffix = suffix;
}

void KernelArchive::extract(Progress *progress) const {
    if (!std::filesystem::exists(_path)) {
        throw std::runtime_error("You must download the archive first!");
    }

    const std::string destination = Kernel::get_source_directory(_kernelVersion);

    std::filesystem::create_directories(destination);

    archive *archive = archive_read_new();
    struct archive *ext = archive_write_disk_new();
    archive_entry *entry;

    constexpr int flags = ARCHIVE_EXTRACT_TIME
                          | ARCHIVE_EXTRACT_PERM
                          | ARCHIVE_EXTRACT_ACL
                          | ARCHIVE_EXTRACT_FFLAGS;

    archive_read_support_filter_all(archive);
    archive_read_support_format_all(archive);
    archive_write_disk_set_options(ext, flags);
    archive_write_disk_set_standard_lookup(ext);

    if (archive_read_open_filename(archive, _path.c_str(), 10240)) {
        throw std::runtime_error(archive_error_string(archive));
    }

    const size_t stripLength = _kernelVersion.get_string(Kernel::Version::NO_OPTIONS).length() + 7;

    for (;;) {
        const int status = archive_read_next_header(archive, &entry);\

        if (status == ARCHIVE_EOF) {
            break;
        }
        if (status != ARCHIVE_OK) {
            throw std::runtime_error(archive_error_string(archive));
        }

        std::string entryPath = archive_entry_pathname(entry);
        std::string path = destination + "/" + entryPath.substr(stripLength);

        archive_entry_set_pathname(entry, path.c_str());

        if (archive_write_header(ext, entry) != ARCHIVE_OK) {
            throw std::runtime_error(archive_error_string(ext));
        }

        copy_data(archive, ext);

        if (archive_write_finish_entry(ext) != ARCHIVE_OK) {
            throw std::runtime_error(archive_error_string(ext));
        }

        if (progress) {
            // Counting entries first would mean decompressing twice.
            progress->set_completed(archive_filter_bytes(archive, -1));

            if (progress->is_cancel_requested()) {
                break;
            }
        }
    }

    archive_read_close(archive);
    archive_read_free(archive);

    archive_write_close(ext);
    archive_write_free(ext);
}

void KernelArchive::remove() const {
    std::filesystem::remove(_path);
}

std::vector<KernelArchive> KernelArchive::list_archives() {
    std::vector<KernelArchive> archives;
    const std::string extension = "." + Configuration::get()->archiveFormat;

    // A missing archive directory is an empty list, not an error.
    std::error_code error;

    for (std::filesystem::directory_iterator it(Configuration::get()->archiveDirectory, error), end;
            !error && it != end; it.increment(error)) {
        std::string filename = it->path().filename();

        if (!filename.starts_with("linux-") || !filename.ends_with(extension)) {
            continue;
        }

        try {
            archives.emplace_back(filename.substr(0, filename.length() - extension.length()), it->path());
        } catch (const std::exception &) {
            // Not a version, not an archive.
        }
    }

    std::ranges::sort(archives, [](const KernelArchive &a, const KernelArchive &b) {
        return a.get_version() > b.get_version();
    });

    return archives;
}

std::string KernelArchive::get_url(const Kernel::Version &version) {
    const Settings *config = Configuration::get();
    std::string family = std::format("v{}.x", version.major);

    return std::format("{}/{}/linux-{}.{}",
                       config->kernelCdn,
                       family,
                       version.get_string(Kernel::Version::NO_OPTIONS),
                       config->archiveFormat);
}

std::string KernelArchive::get_path(const Kernel::Version &version) {
    const Settings *config = Configuration::get();

    return std::format("{}/linux-{}.{}",
                       config->archiveDirectory,
                       version.get_string(Kernel::Version::NO_OPTIONS),
                       config->archiveFormat);
}

static int copy_data(archive *read, archive *write)
{
    int status;
    const void *buff;
    size_t size;
#if ARCHIVE_VERSION_NUMBER >= 3000000
    int64_t offset;
#else
    off_t offset;
#endif

    for (;;) {
        status = archive_read_data_block(read, &buff, &size, &offset);
        if (status == ARCHIVE_EOF) {
            return ARCHIVE_OK;
        }
        if (status != ARCHIVE_OK) {
            return status;
        }

        if (archive_write_data_block(write, buff, size, offset) != ARCHIVE_OK) {
            return status;
        }
    }
}
