// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2024
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <filesystem>
#include "ExtractTask.h"
#include "core/Kernel.h"

ExtractTask::ExtractTask(KernelArchive *archive) : Task("Extract task", std::make_unique<Progress>()){
    _archive = archive;
}

ExtractTask::ExtractTask(const Kernel::Version &version)
        : Task("Extract task", std::make_unique<Progress>()),
          _owned(std::make_unique<KernelArchive>(version)) {
    _archive = _owned.get();
}

bool ExtractTask::run() {
    bool canceled = false;

    try {
        const std::string destination = Kernel::get_source_directory(_archive->get_version());

        step("Extracting " + _archive->get_name() + " to " + destination);

        if (std::filesystem::exists(destination)) {
            report("Replacing existing directory");
            std::filesystem::remove_all(destination);
        }

        _progress->set_total(std::filesystem::file_size(_archive->get_location()));
        _archive->extract(_progress.get());

        canceled = _progress->is_cancel_requested();

        if (canceled) {
            std::filesystem::remove_all(destination);
        }
    } catch (const std::exception &ex) {
        canceled = true;
        fail(ex.what());
    }

    _progress->end(canceled);
    step_finished(!canceled);

    if (!canceled && _on_complete) {
        _on_complete();
    }

    return !canceled;
}

int ExtractTask::position() {
    return 2;
}

void ExtractTask::cancel() {
    _progress->request_cancel();
}
