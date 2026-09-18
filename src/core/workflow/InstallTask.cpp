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
#include "InstallTask.h"
#include "core/Command.h"
#include "core/Configuration.h"
#include "core/Make.h"

InstallTask::InstallTask(Kernel::Version version, const Toolchain compiler)
        : ProcessTask("Install task", 6), _version(std::move(version)), _compiler(compiler) {
}

bool InstallTask::prepare() {
    const Settings *settings = Configuration::get();
    const std::string source = Kernel::get_source_directory(_version);
    const std::string version = _version.get_string();
    const std::string boot = settings->bootDirectory;

    if (!std::filesystem::exists(source)) {
        fail("Kernel directory " + source + " does not exist!");

        return false;
    }

    std::string image = Command("make -s --no-print-directory -C '" + source + "' image_name 2>/dev/null").capture();

    if (image.empty()) {
        image = "arch/x86/boot/bzImage";
    }

    if (!std::filesystem::exists(source + "/" + image)) {
        fail("No kernel image at " + source + "/" + image + ", build the kernel first!");

        return false;
    }

    _steps.push_back(Step {
        .label = "Installing modules",
        .command = Make::command("modules_install", _compiler),
        .directory = source,
        .elevated = true,
        .optional = false
    });

    _steps.push_back(Step {
        .label = "Installing image to " + boot + "/vmlinuz-" + version,
        .command = "install -Dvm644 '" + source + "/" + image + "' '" + boot + "/vmlinuz-" + version + "'"
            + " && install -Dvm644 '" + source + "/System.map' '" + boot + "/System.map-" + version + "'"
            + " && install -Dvm644 '" + source + "/.config' '" + boot + "/config-" + version + "'",
        .directory = source,
        .elevated = true,
        .optional = false
    });

    if (Command::exists("mkinitcpio")) {
        const std::string preset = "/etc/mkinitcpio.d/" + version + ".preset";

        _steps.push_back(Step {
            .label = "Creating mkinitcpio preset " + preset,
            .command = "printf '%s\n' "
                "'ALL_config=\"/etc/mkinitcpio.conf\"' "
                "'ALL_kver=\"" + boot + "/vmlinuz-" + version + "\"' "
                "'PRESETS=(\"default\" \"fallback\")' "
                "'default_image=\"" + boot + "/initramfs-" + version + ".img\"' "
                "'fallback_image=\"" + boot + "/initramfs-" + version + "-fallback.img\"' "
                "'fallback_options=\"-S autodetect\"' "
                "> '" + preset + "'",
            .directory = source,
            .elevated = true,
            .optional = false
        });

        _steps.push_back(Step {
            .label = "Generating initramfs",
            .command = "mkinitcpio -p '" + version + "'",
            .directory = source,
            .elevated = true,
            .optional = false
        });
    } else if (Command::exists("dracut")) {
        _steps.push_back(Step {
            .label = "Generating initramfs",
            .command = "dracut --force --kver '" + version + "' '" + boot + "/initramfs-" + version + ".img'",
            .directory = source,
            .elevated = true,
            .optional = false
        });
    } else {
        report("Neither mkinitcpio nor dracut is installed, skipping initramfs generation.");
    }

    if (Command::exists("dkms")) {
        _steps.push_back(Step {
            .label = "Installing dkms modules",
            .command = "dkms autoinstall -k '" + version + "'",
            .directory = source,
            .elevated = true,
            .optional = true
        });
    }

    if (Command::exists("grub-mkconfig")) {
        _steps.push_back(Step {
            .label = "Refreshing grub",
            .command = "grub-mkconfig -o '" + settings->grubDirectory + "/grub.cfg'",
            .directory = source,
            .elevated = true,
            .optional = true
        });
    }

    return true;
}
