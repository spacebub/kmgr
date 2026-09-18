// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2024
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <filesystem>
#include "DownloadTask.h"

#include "core/Download.h"

DownloadTask::DownloadTask(const std::string &url,
                           const std::string &destination)
                           : Task("Download task", std::make_unique<Progress>()) {
    _url = url;
    _destination = destination;
}

bool DownloadTask::run() {
    const Download download(_url);
    bool canceled = false;

    step("Downloading " + _url);

    try {
        std::filesystem::create_directories(std::filesystem::path(_destination).parent_path());
        download.perform(_destination, _progress.get());
    } catch (const std::exception &ex) {
        canceled = true;

        std::filesystem::remove(_destination);
        fail(ex.what());
    }

    _progress->end(canceled);
    step_finished(!canceled);

    if (!canceled && _on_complete) {
        _on_complete();
    }

    return !canceled;
}

int DownloadTask::position() {
    return 1;
}

void DownloadTask::cancel() {
    _progress->request_cancel();
}
