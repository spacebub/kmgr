// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <filesystem>

#include "core/Configuration.h"
#include "core/Elevation.h"
#include "core/Make.h"
#include "gui/model/SettingsBridge.h"
#include "ttk/system/Text.h"

SettingsBridge::SettingsBridge(ttk::Notifier *notifier) : _notifier(notifier) {
    load();
}

int SettingsBridge::detected_jobs() {
    return Make::jobs();
}

std::string SettingsBridge::aocc_preset() {
    return R"(LLVM=/path/to/aocc/bin/ KCFLAGS="-march=native -mtune=native" )"
           R"(HOSTCFLAGS="-march=native -mtune=native")";
}

bool SettingsBridge::password_remembered() {
    return Elevation::remembered();
}

void SettingsBridge::forget_password() {
    Elevation::forget();

    _notifier->info("The password will be asked for again on the next step that needs it.");

    if (changed) {
        changed();
    }
}

bool SettingsBridge::notifications_answered() {
    return Configuration::get()->notifications.has_value();
}

void SettingsBridge::set_notifications(const bool enabled) {
    Settings settings = *Configuration::get();
    settings.notifications = enabled;

    Configuration::set(settings);

    try {
        Configuration::save();
    } catch (const std::exception &ex) {
        _notifier->error(ex.what(), "Setting not saved");

        return;
    }

    notifications = enabled;
    _revision++;

    if (changed) {
        changed();
    }
}

void SettingsBridge::load() {
    const Settings *settings = Configuration::get();

    bootDirectory = settings->bootDirectory;
    grubDirectory = settings->grubDirectory;
    archiveFormat = settings->archiveFormat;
    kernelCdn = settings->kernelCdn;
    baseDirectory = settings->baseDirectory;
    archiveDirectory = settings->archiveDirectory;
    elevationCommand = settings->elevationCommand;
    jobs = settings->jobs;
    compiler = toolchain_name(settings->compiler);
    customToolchainName = settings->customToolchainName;
    customFlags = settings->customFlags;
    notifications = settings->notifications.value_or(false);
    _revision++;

    if (changed) {
        changed();
    }
}

// The configuration only takes directories that exist, so missing ones are
// created first rather than quietly dropped.
bool SettingsBridge::save() {
    const std::string directories[] = { bootDirectory, grubDirectory, baseDirectory, archiveDirectory };

    for (const std::string &directory : directories) {
        if (directory.empty()) {
            _notifier->error("Directories cannot be left empty.", "Settings not saved");

            return false;
        }

        std::error_code error;
        std::filesystem::create_directories(directory, error);

        if (!std::filesystem::is_directory(directory)) {
            _notifier->error(directory + " is not a directory and could not be created.", "Settings not saved");

            return false;
        }
    }

    const Settings settings {
        .bootDirectory = bootDirectory,
        .grubDirectory = grubDirectory,
        .archiveFormat = ttk::Text::trim(archiveFormat),
        .kernelCdn = ttk::Text::trim(kernelCdn),
        .baseDirectory = baseDirectory,
        .archiveDirectory = archiveDirectory,
        .elevationCommand = ttk::Text::trim(elevationCommand),
        .jobs = jobs,
        .colors = Configuration::get()->colors,
        .compiler = toolchain_from(compiler),
        .customToolchainName = ttk::Text::trim(customToolchainName),
        .customFlags = ttk::Text::trim(customFlags),
        .theme = Configuration::get()->theme,
        .notifications = notifications
    };

    Configuration::set(settings);

    try {
        Configuration::save();
    } catch (const std::exception &ex) {
        _notifier->error(ex.what(), "Settings not saved");

        return false;
    }

    load();

    _notifier->success("Settings saved.");

    if (saved) {
        saved();
    }

    return true;
}
