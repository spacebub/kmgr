// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <filesystem>
#include <stdexcept>

#include "WorkflowFactory.h"
#include "BuildTask.h"
#include "CleanTask.h"
#include "ConfigureTask.h"
#include "DownloadTask.h"
#include "ExtractTask.h"
#include "InstallTask.h"
#include "PatchTask.h"
#include "PullTask.h"
#include "SignTask.h"
#include "core/Configuration.h"
#include "core/KernelArchive.h"
#include "core/SystemInfo.h"

namespace {
    // Versions end up in commands that run as root.
    void validate(const std::string &value, const std::string &description) {
        if (value.find_first_not_of("abcdefghijklmnopqrstuvwxyz"
                                    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                    "0123456789._+-") != std::string::npos) {
            throw std::invalid_argument("Invalid " + description + ": " + value);
        }
    }

    // A run of a single step is filed under that step, anything larger is not.
    std::string operation_of(const int stages) {
        switch (stages & ~(Options::CLANG | Options::CUSTOM | Options::FORCE)) {
            case Options::PULL: return "pull";
            case Options::DOWNLOAD: return "download";
            case Options::EXTRACT: return "extract";
            case Options::PATCH: return "patch";
            case Options::REVERT: return "revert";
            case Options::CONFIGURE: return "configure";
            case Options::BUILD: return "build";
            case Options::INSTALL: return "install";
            case Options::SIGN: return "sign";
            case Options::MAKE: return "build";
            case Options::CLEAN_ARCHIVE: return "archive";
            case Options::CLEAN_SOURCE: return "sources";
            case Options::CLEAN_INSTALLED:
            case Options::CLEAN_INSTALLED | Options::CLEAN_SOURCE: return "remove";
            default: return {};
        }
    }

    Kernel::Version resolve(const std::string &version, const std::string &suffix) {
        validate(version, "kernel version");
        validate(suffix, "kernel suffix");

        Kernel::Version resolved = Kernel::get_version(version);
        resolved.suffix = suffix;

        return resolved;
    }
}

Workflow *WorkflowFactory::create(const Options &options) {
    const Toolchain compiler = options.toolchain();
    const bool cleaning = (options.stages & (Options::CLEAN_INSTALLED | Options::CLEAN_SOURCE | Options::CLEAN_ARCHIVE)) != 0;

    if (options.kernel.empty() && !cleaning && options.stages != Options::PULL) {
        throw std::invalid_argument("No kernel version specified!");
    }

    Kernel::Version version{};

    if (!options.kernel.empty()) {
        version = resolve(options.kernel, options.suffix);
    }

    auto *workflow = new Workflow(options.kernel.empty()
        ? "Maintenance"
        : "Kernel " + version.get_string());

    workflow->set_force((options.stages & Options::FORCE) != 0);

    if (options.stages & Options::PULL) {
        workflow->queue(new PullTask());
    }

    if (options.stages & Options::DOWNLOAD) {
        KernelArchive archive(version);

        if (!std::filesystem::exists(archive.get_location())) {
            workflow->queue(new DownloadTask(archive.get_url(), archive.get_location()));
        }
    }

    if (options.stages & Options::EXTRACT) {
        workflow->queue(new ExtractTask(version));
    }

    if (options.stages & Options::PATCH) {
        workflow->queue(new PatchTask(version, options.patch));
    }

    if (options.stages & Options::REVERT) {
        workflow->queue(new PatchTask(version, options.patch, PatchTask::Reverse));
    }

    if (options.stages & Options::CONFIGURE) {
        workflow->queue(new ConfigureTask(version, options.config, compiler));
    }

    if (options.stages & Options::BUILD) {
        workflow->queue(new BuildTask(version, compiler));
    }

    if (options.stages & Options::INSTALL) {
        workflow->queue(new InstallTask(version, compiler));
    }

    if (options.stages & Options::SIGN) {
        workflow->queue(new SignTask(version));
    }

    if (cleaning) {
        // A replaced kernel carries the suffix of the new one unless given one of its own.
        const Kernel::Version target = options.oldKernel.empty()
            ? version
            : resolve(options.oldKernel, options.oldSuffix.empty() ? options.suffix : options.oldSuffix);
        int mode = 0;

        if (options.stages & Options::CLEAN_INSTALLED) {
            mode |= CleanTask::Installed;
        }
        if (options.stages & Options::CLEAN_SOURCE) {
            mode |= CleanTask::Source;
        }
        if (options.stages & Options::CLEAN_ARCHIVE) {
            mode |= CleanTask::Archive;
        }

        workflow->queue(new CleanTask(target, mode));
    }

    workflow->set_context(options.kernel.empty() ? std::string{} : version.get_string(),
                          operation_of(options.stages));
    workflow->sort();

    return workflow;
}

Options WorkflowFactory::autoupdate() {
    const Kernel::Version current = Kernel::get_current().get_version();
    const Kernel::Version latest = Kernel::get_latest().get_version();

    if (latest <= current) {
        throw std::runtime_error("No newer kernel available, "
            + current.get_string() + " is already up to date.");
    }

    const auto [running, sbctlStatus] = SystemInfo::get();

    Options options;
    options.kernel = latest.get_string(Kernel::Version::INCLUDE_ZERO_REVISION);
    options.suffix = current.suffix;
    options.oldKernel = current.get_string(Kernel::Version::INCLUDE_ZERO_REVISION);
    options.oldSuffix = current.suffix;
    options.stages = Options::FULL | Options::CLEAN_INSTALLED | Options::CLEAN_SOURCE;

    // The running kernel's banner cannot say "custom", so the setting decides first.
    if (Configuration::get()->compiler == Toolchain::Custom) {
        options.stages |= Options::CUSTOM;
    } else if (running == Toolchain::Llvm) {
        options.stages |= Options::CLANG;
    }

    if (sbctlStatus == Present) {
        options.stages |= Options::SIGN;
    }

    return options;
}
