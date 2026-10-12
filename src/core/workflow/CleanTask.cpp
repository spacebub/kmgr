// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "CleanTask.h"
#include "core/Command.h"
#include "core/Configuration.h"
#include "core/Dkms.h"
#include "core/KernelArchive.h"
#include "core/Shell.h"
#include <filesystem>
#include <utility>

CleanTask::CleanTask(Kernel::Version version, const int mode)
    : ProcessTask("Clean task", 8), _version(std::move(version)), _mode(mode) {
}

bool CleanTask::prepare() {
    const Settings *settings = Configuration::get();
    const bool versioned = _version.major > 0;
    const std::string version = _version.get_string();
    const std::string boot = settings->bootDirectory;

    if (_mode & Installed) {
        if (!versioned) {
            fail("Cleaning installed files requires a kernel version!");

            return false;
        }

        const Kernel kernel(version);

        // Files a package manager put there are its to take away, or its records go wrong.
        if (const std::string package = kernel.packaged_by(); !package.empty()) {
            fail(version + " was installed by the package " + package
                 + ", so it is for the package manager to remove.");

            return false;
        }

        if (Dkms::available()) {
            _steps.push_back(Step{.label = "Removing dkms modules built for " + version,
                                  .command = Dkms::removal(version),
                                  .directory = settings->baseDirectory,
                                  .elevated = true,
                                  .optional = true,
                                  .undo = {}});
        }

        _steps.push_back(Step{.label = "Removing modules",
                              .command = "rm -rfv " + Shell::quote("/usr/lib/modules/" + version) + " "
                                         + Shell::quote("/lib/modules/" + version),
                              .directory = settings->baseDirectory,
                              .elevated = true,
                              .optional = false,
                              .undo = {}});

        if (Command::exists("sbctl")) {
            _steps.push_back(Step{.label = "Removing image signature",
                                  .command = "sbctl remove-file " + Shell::quote(boot + "/vmlinuz-" + version),
                                  .directory = settings->baseDirectory,
                                  .elevated = true,
                                  .optional = true,
                                  .undo = {}});
        }

        std::string files;

        for (const std::string &file : {boot + "/vmlinuz-" + version,
                                        boot + "/initramfs-" + version + ".img",
                                        boot + "/initramfs-" + version + "-fallback.img",
                                        boot + "/System.map-" + version,
                                        boot + "/config-" + version,
                                        "/etc/mkinitcpio.d/" + version + ".preset"}) {
            files += " " + Shell::quote(file);
        }

        _steps.push_back(Step{.label = "Removing boot files",
                              .command = "rm -fv" + files,
                              .directory = settings->baseDirectory,
                              .elevated = true,
                              .optional = false,
                              .undo = {}});

        // Once the files are gone grub has to be told, or its menu offers a kernel that is not there.
        if (Command::exists("grub-mkconfig")) {
            const std::string config = settings->grubDirectory + "/grub.cfg";

            if (std::filesystem::exists(config)) {
                _steps.push_back(Step{.label = "Refreshing grub",
                                      .command = "grub-mkconfig -o " + Shell::quote(config),
                                      .directory = settings->baseDirectory,
                                      .elevated = true,
                                      .optional = false,
                                      .undo = {}});
            } else {
                report("No " + config + ", so grub is not refreshed.");
            }
        }
    }

    if (_mode & Source) {
        if (!versioned) {
            fail("Removing a build directory requires a kernel version!");

            return false;
        }

        const std::string source = Kernel::get_source_directory(_version);

        if (std::filesystem::exists(source)) {
            _steps.push_back(Step{.label = "Removing build directory " + source,
                                  .command = "rm -rf " + Shell::quote(source),
                                  .directory = settings->baseDirectory,
                                  .elevated = false,
                                  .optional = false,
                                  .undo = {}});
        } else {
            report("No build directory at " + source + ".");
        }
    }

    if (_mode & Archive) {
        std::string archives;

        // A download cut short by a killed process leaves its working file, which goes too.
        if (versioned) {
            const std::string location = KernelArchive(_version).get_location();

            archives = " " + Shell::quote(location) + " " + Shell::quote(location + ".part");
        } else {
            for (const KernelArchive &archive : KernelArchive::list_archives()) {
                archives += " " + Shell::quote(archive.get_location()) + " "
                            + Shell::quote(archive.get_location() + ".part");
            }
        }

        if (archives.empty()) {
            report("No archives to remove.");
        } else {
            _steps.push_back(Step{.label = "Removing archives",
                                  .command = "rm -fv" + archives,
                                  .directory = settings->baseDirectory,
                                  .elevated = false,
                                  .optional = false,
                                  .undo = {}});
        }
    }

    return true;
}
