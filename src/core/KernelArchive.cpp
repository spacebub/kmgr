// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2024
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "KernelArchive.h"
#include "Configuration.h"
#include "archive.h"
#include "archive_entry.h"
#include <algorithm>
#include <filesystem>
#include <format>
#include <memory>
#include <stdexcept>

namespace {
using Handle = std::unique_ptr<archive, int (*)(archive *)>;

// Every entry has to sit under the top directory the archive is named after, and nowhere
// else, before its path is used for anything.
std::string relocate(const std::string &entry, const std::string &top, const std::string &destination) {
    if (entry == top || entry == top + "/") {
        return destination;
    }

    if (!entry.starts_with(top + "/")) {
        throw std::runtime_error("Unexpected entry " + entry + " in the archive");
    }

    const std::filesystem::path relative(entry.substr(top.size() + 1));

    for (const std::filesystem::path &part : relative) {
        if (part == "..") {
            throw std::runtime_error("Entry " + entry + " in the archive reaches outside its directory");
        }
    }

    return destination + "/" + relative.string();
}

int copy_data(archive *read, archive *write) {
    const void *buffer;
    size_t size;
    int64_t offset;

    for (;;) {
        const int status = archive_read_data_block(read, &buffer, &size, &offset);

        if (status == ARCHIVE_EOF) {
            return ARCHIVE_OK;
        }

        if (status != ARCHIVE_OK) {
            return status;
        }

        if (const int written = archive_write_data_block(write, buffer, size, offset); written != ARCHIVE_OK) {
            return written;
        }
    }
}
}

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

std::string KernelArchive::get_url() const {
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

std::string KernelArchive::get_checksum_url() const {
    return std::format("{}/v{}.x/sha256sums.asc", Configuration::get()->kernelCdn, _kernelVersion.major);
}

// Lines read "<64 hex digits>  <name>" between the clearsign headers and the signature.
std::string KernelArchive::find_checksum(const std::string &listing, const std::string &name) {
    constexpr size_t DIGEST = 64;
    size_t start = 0;

    while (start < listing.size()) {
        const size_t end = listing.find('\n', start);
        const std::string_view line(listing.data() + start, (end == std::string::npos ? listing.size() : end) - start);

        start = end == std::string::npos ? listing.size() : end + 1;

        if (line.size() != DIGEST + 2 + name.size() || line.substr(DIGEST, 2) != "  "
            || line.substr(DIGEST + 2) != name) {
            continue;
        }

        const std::string_view digest = line.substr(0, DIGEST);

        if (std::ranges::all_of(digest, [](const char character) {
                return (character >= '0' && character <= '9') || (character >= 'a' && character <= 'f');
            })) {
            return std::string(digest);
        }
    }

    return {};
}

void KernelArchive::with_suffix(const std::string &suffix) {
    _kernelVersion.suffix = suffix;
}

void KernelArchive::extract(Progress *progress) const {
    if (!std::filesystem::exists(_path)) {
        throw std::runtime_error("You must download the archive first!");
    }

    const std::string top = get_name(false);

    std::filesystem::create_directories(Kernel::get_source_directory(_kernelVersion));

    // Resolved first, so the checks below only ever see what the archive itself asks for.
    const std::string destination = std::filesystem::canonical(Kernel::get_source_directory(_kernelVersion));

    const Handle reader(archive_read_new(), archive_read_free);
    const Handle writer(archive_write_disk_new(), archive_write_free);
    archive_entry *entry;

    // Owners, permissions beyond the mode and flags are not the archive's to set, and neither
    // is anything outside the directory it is unpacked into.
    constexpr int flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_SECURE_NODOTDOT | ARCHIVE_EXTRACT_SECURE_SYMLINKS;

    archive_read_support_filter_all(reader.get());
    archive_read_support_format_all(reader.get());
    archive_write_disk_set_options(writer.get(), flags);
    archive_write_disk_set_standard_lookup(writer.get());

    if (archive_read_open_filename(reader.get(), _path.c_str(), 10240)) {
        throw std::runtime_error(archive_error_string(reader.get()));
    }

    for (;;) {
        const int status = archive_read_next_header(reader.get(), &entry);

        if (status == ARCHIVE_EOF) {
            break;
        }
        if (status != ARCHIVE_OK) {
            throw std::runtime_error(archive_error_string(reader.get()));
        }

        const std::string path = relocate(archive_entry_pathname(entry), top, destination);

        if (path == destination) {
            archive_read_data_skip(reader.get());

            continue;
        }

        archive_entry_set_pathname(entry, path.c_str());

        if (const char *hardlink = archive_entry_hardlink(entry)) {
            archive_entry_set_hardlink(entry, relocate(hardlink, top, destination).c_str());
        }

        if (archive_write_header(writer.get(), entry) != ARCHIVE_OK) {
            throw std::runtime_error(archive_error_string(writer.get()));
        }

        if (copy_data(reader.get(), writer.get()) != ARCHIVE_OK) {
            throw std::runtime_error(std::string("Could not unpack ") + archive_entry_pathname(entry) + ": "
                                     + (archive_error_string(reader.get()) ? archive_error_string(reader.get())
                                                                           : archive_error_string(writer.get())));
        }

        if (archive_write_finish_entry(writer.get()) != ARCHIVE_OK) {
            throw std::runtime_error(archive_error_string(writer.get()));
        }

        if (progress) {
            // Counting entries first would mean decompressing twice.
            progress->set_completed(archive_filter_bytes(reader.get(), -1));

            if (progress->is_cancel_requested()) {
                break;
            }
        }
    }

    archive_read_close(reader.get());
    archive_write_close(writer.get());
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
         !error && it != end;
         it.increment(error)) {
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

    std::ranges::sort(archives,
                      [](const KernelArchive &a, const KernelArchive &b) { return a.get_version() > b.get_version(); });

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
