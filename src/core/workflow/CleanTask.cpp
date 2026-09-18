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
#include "CleanTask.h"
#include "core/Command.h"
#include "core/Configuration.h"
#include "core/KernelArchive.h"

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

        if (Command::exists("dkms")) {
            _steps.push_back(Step {
                .label = "Removing dkms modules built for " + version,
                .command = "dkms status -k '" + version + "' | while IFS= read -r entry; do "
                    "module=$(printf '%s' \"$entry\" | cut -d',' -f1); "
                    "[ -n \"$module\" ] && dkms remove \"$module\" -k '" + version + "'; "
                    "done; rm -rfv /var/lib/dkms/*/kernel-" + version + "-*",
                .directory = settings->baseDirectory,
                .elevated = true,
                .optional = true
            });
        }

        _steps.push_back(Step {
            .label = "Removing modules",
            .command = "rm -rfv '/usr/lib/modules/" + version + "' '/lib/modules/" + version + "'",
            .directory = settings->baseDirectory,
            .elevated = true,
            .optional = false
        });

        if (Command::exists("sbctl")) {
            _steps.push_back(Step {
                .label = "Removing image signature",
                .command = "sbctl remove-file '" + boot + "/vmlinuz-" + version + "'",
                .directory = settings->baseDirectory,
                .elevated = true,
                .optional = true
            });
        }

        std::string files = "'" + boot + "/vmlinuz-" + version + "'"
            " '" + boot + "/initramfs-" + version + ".img'"
            " '" + boot + "/initramfs-" + version + "-fallback.img'"
            " '" + boot + "/System.map-" + version + "'"
            " '" + boot + "/config-" + version + "'"
            " '/etc/mkinitcpio.d/" + version + ".preset'";

        // A kernel installed by the distribution names its files after the package.
        const Kernel kernel(version);

        if (const std::string image = kernel.get_image();
                !image.empty() && image != boot + "/vmlinuz-" + version) {
            files += " '" + image + "'";
        }

        if (const std::string initramfs = kernel.get_initramfs();
                !initramfs.empty() && initramfs != boot + "/initramfs-" + version + ".img") {
            files += " '" + initramfs + "'";
        }

        _steps.push_back(Step {
            .label = "Removing boot files",
            .command = "rm -fv " + files,
            .directory = settings->baseDirectory,
            .elevated = true,
            .optional = false
        });

        if (Command::exists("grub-mkconfig")) {
            _steps.push_back(Step {
                .label = "Refreshing grub",
                .command = "grub-mkconfig -o '" + settings->grubDirectory + "/grub.cfg'",
                .directory = settings->baseDirectory,
                .elevated = true,
                .optional = true
            });
        }
    }

    if (_mode & Source) {
        if (!versioned) {
            fail("Removing a build directory requires a kernel version!");

            return false;
        }

        const std::string source = Kernel::get_source_directory(_version);

        if (std::filesystem::exists(source)) {
            _steps.push_back(Step {
                .label = "Removing build directory " + source,
                .command = "rm -rf '" + source + "'",
                .directory = settings->baseDirectory,
                .elevated = false,
                .optional = false
            });
        } else {
            report("No build directory at " + source + ".");
        }
    }

    if (_mode & Archive) {
        std::string archives;

        if (versioned) {
            archives = "'" + KernelArchive(_version).get_location() + "'";
        } else {
            for (const KernelArchive &archive : KernelArchive::list_archives()) {
                archives += " '" + archive.get_location() + "'";
            }
        }

        if (archives.empty()) {
            report("No archives to remove.");
        } else {
            _steps.push_back(Step {
                .label = "Removing archives",
                .command = "rm -fv " + archives,
                .directory = settings->baseDirectory,
                .elevated = false,
                .optional = false
            });
        }
    }

    return true;
}
