// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2024
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "DownloadTask.h"
#include <filesystem>
#include <utility>

#include "core/Download.h"
#include "core/Sha256.h"

DownloadTask::DownloadTask(KernelArchive archive)
    : Task("Download task", std::make_unique<Progress>()), _archive(std::move(archive)) {
}

bool DownloadTask::run() {
    const std::string destination = _archive.get_location();
    const std::string partial = destination + ".part";
    bool success = false;

    try {
        step("Downloading " + _archive.get_url());

        std::filesystem::create_directories(std::filesystem::path(destination).parent_path());
        Download(_archive.get_url()).perform(partial, _progress.get());
        step_finished(true);

        step("Verifying " + _archive.get_name());
        success = verify(partial);
        step_finished(success);
    } catch (const std::exception &ex) {
        if (_progress->is_cancel_requested()) {
            report("Canceled.");
        } else {
            fail(ex.what());
        }

        step_finished(false);
    }

    std::error_code error;

    if (success) {
        std::filesystem::rename(partial, destination, error);

        if (error) {
            fail("Could not move " + partial + " into place: " + error.message());
            success = false;
        }
    }

    if (!success) {
        std::filesystem::remove(partial, error);
    }

    _progress->end(!success);

    if (success && _on_complete) {
        _on_complete();
    }

    return success;
}

bool DownloadTask::verify(const std::string &file) {
    const std::string name = _archive.get_name();
    const std::string listing = Download(_archive.get_checksum_url()).get_page();
    const std::string expected = KernelArchive::find_checksum(listing, name);

    if (expected.empty()) {
        fail(_archive.get_checksum_url() + " has no entry for " + name);

        return false;
    }

    const std::string actual = Sha256::file(file);

    if (actual != expected) {
        fail("The digest of " + name + " is " + actual + ", where " + _archive.get_checksum_url() + " says "
             + expected);

        return false;
    }

    report("sha256 " + actual + " matches the published digest");

    return true;
}

int DownloadTask::position() {
    return 1;
}

void DownloadTask::cancel() {
    _progress->request_cancel();
}
