// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <filesystem>
#include "PullTask.h"
#include "core/Configuration.h"

PullTask::PullTask() : ProcessTask("Pull task", 0) {
}

bool PullTask::prepare() {
    const std::string base = Configuration::get()->baseDirectory;

    for (const auto &entry : std::filesystem::directory_iterator(base)) {
        if (!entry.is_directory() || !std::filesystem::exists(entry.path() / ".git")) {
            continue;
        }

        _steps.push_back(Step {
            .label = "Updating " + entry.path().filename().string(),
            .command = "git pull --ff-only",
            .directory = entry.path(),
            .elevated = false,
            .optional = true
        });
    }

    if (_steps.empty()) {
        report("No repositories found in " + base);
    }

    return true;
}
