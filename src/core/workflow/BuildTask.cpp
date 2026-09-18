// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <filesystem>
#include <utility>
#include "BuildTask.h"
#include "core/Make.h"

BuildTask::BuildTask(Kernel::Version version, const Toolchain compiler)
        : ProcessTask("Build task", 5), _version(std::move(version)), _compiler(compiler) {
}

bool BuildTask::prepare() {
    const std::string source = Kernel::get_source_directory(_version);

    if (!std::filesystem::exists(source)) {
        fail("Kernel directory " + source + " does not exist!");

        return false;
    }

    if (!std::filesystem::exists(source + "/.config")) {
        fail("No configuration in " + source + ", configure the kernel first!");

        return false;
    }

    _steps.push_back(Step {
        .label = "Building with " + std::to_string(Make::jobs()) + " jobs",
        .command = Make::command({}, _compiler),
        .directory = source,
        .elevated = false,
        .optional = false
    });

    return true;
}
