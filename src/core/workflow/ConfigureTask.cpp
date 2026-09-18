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
#include "ConfigureTask.h"
#include "core/Configuration.h"
#include "core/Make.h"

ConfigureTask::ConfigureTask(Kernel::Version version, std::string config, const Toolchain compiler)
        : ProcessTask("Configure task", 4), _version(std::move(version)), _config(std::move(config)),
          _compiler(compiler) {
}

std::string ConfigureTask::find_config() const {
    if (_config.empty()) {
        return Kernel::find_config(_version);
    }

    if (std::filesystem::exists(_config)) {
        return _config;
    }

    const std::filesystem::path supplied = std::filesystem::path(Configuration::get()->baseDirectory) / _config;

    return std::filesystem::exists(supplied) ? supplied.string() : std::string{};
}

// oldconfig runs last whatever the source, so it asks about anything the new kernel added.
bool ConfigureTask::prepare() {
    const std::string source = Kernel::get_source_directory(_version);

    if (!std::filesystem::exists(source)) {
        fail("Kernel directory " + source + " does not exist!");

        return false;
    }

    const std::string config = find_config();
    const bool present = std::filesystem::exists(source + "/.config");

    if (config.empty() && !present) {
        report("No configuration found, generating a default one.");

        _steps.push_back(Step {
            .label = "Generating default configuration",
            .command = Make::command("defconfig", _compiler, false),
            .directory = source,
            .elevated = false,
            .optional = false
        });
    } else if (!config.empty()) {
        _steps.push_back(Step {
            .label = "Using configuration " + config,
            .command = config.ends_with(".gz")
                ? "zcat '" + config + "' > .config"
                : "cp -v '" + config + "' .config",
            .directory = source,
            .elevated = false,
            .optional = false
        });
    } else {
        report("Reusing the configuration already present in the build directory.");
    }

    std::string tweaks = "./scripts/config --file .config --disable LOCALVERSION_AUTO";

    if (!_version.suffix.empty()) {
        tweaks += " --set-str LOCALVERSION '-" + _version.suffix + "'";
    } else {
        tweaks += " --set-str LOCALVERSION ''";
    }

    // Distribution keys are not available outside of their own build.
    tweaks += " --set-str SYSTEM_TRUSTED_KEYS '' --set-str SYSTEM_REVOCATION_KEYS ''";

    _steps.push_back(Step {
        .label = "Applying local version and key settings",
        .command = tweaks,
        .directory = source,
        .elevated = false,
        .optional = false
    });

    _steps.push_back(Step {
        .label = "Resolving new configuration symbols",
        .command = Make::command("oldconfig", _compiler, false),
        .directory = source,
        .elevated = false,
        .optional = false
    });

    return true;
}
