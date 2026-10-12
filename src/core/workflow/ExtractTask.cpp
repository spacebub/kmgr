// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2024
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "ExtractTask.h"
#include "core/Kernel.h"
#include <filesystem>

ExtractTask::ExtractTask(KernelArchive *archive)
    : Task("Extract task", std::make_unique<Progress>()), _archive(archive) {
}

ExtractTask::ExtractTask(const Kernel::Version &version)
    : Task("Extract task", std::make_unique<Progress>()),
      _owned(std::make_unique<KernelArchive>(version)),
      _archive(_owned.get()) {
}

// A directory left behind by a run that stopped would pass for an unpacked kernel, so it goes.
bool ExtractTask::run() {
    const std::string destination = Kernel::get_source_directory(_archive->get_version());
    bool failed = false;

    try {
        step("Extracting " + _archive->get_name() + " to " + destination);

        if (std::filesystem::exists(destination)) {
            report("Replacing existing directory");
            std::filesystem::remove_all(destination);
        }

        _progress->set_total(std::filesystem::file_size(_archive->get_location()));
        _archive->extract(_progress.get());

        failed = _progress->is_cancel_requested();

        if (failed) {
            report("Canceled.");
        }
    } catch (const std::exception &ex) {
        failed = true;
        fail(ex.what());
    }

    if (failed) {
        std::error_code error;

        std::filesystem::remove_all(destination, error);
    }

    _progress->end(failed);
    step_finished(!failed);

    if (!failed && _on_complete) {
        _on_complete();
    }

    return !failed;
}

int ExtractTask::position() {
    return 2;
}

void ExtractTask::cancel() {
    _progress->request_cancel();
}
