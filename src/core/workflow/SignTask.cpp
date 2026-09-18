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
#include "SignTask.h"
#include "core/Command.h"
#include "core/Configuration.h"

SignTask::SignTask(Kernel::Version version) : ProcessTask("Sign task", 7), _version(std::move(version)) {
}

bool SignTask::prepare() {
    const std::string image = Kernel(_version.get_string()).get_image();

    if (!Command::exists("sbctl")) {
        fail("sbctl is not installed!");

        return false;
    }

    if (image.empty()) {
        fail("No image for " + _version.get_string() + ", install the kernel first!");

        return false;
    }

    _steps.push_back(Step {
        .label = "Signing " + image,
        .command = "sbctl sign -s '" + image + "'",
        .directory = Configuration::get()->baseDirectory,
        .elevated = true,
        .optional = false
    });

    return true;
}
