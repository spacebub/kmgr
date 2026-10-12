// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "InstallTask.h"
#include "core/Command.h"
#include "core/Configuration.h"
#include "core/Dkms.h"
#include "core/Make.h"
#include "core/Shell.h"
#include <filesystem>
#include <utility>

namespace {
// The tree's own makefile names the image, so the name is checked before it goes into a
// command that runs as root.
bool sane_relative_path(const std::string &path) {
    if (path.empty() || path.front() == '/') {
        return false;
    }

    if (path.find_first_not_of("abcdefghijklmnopqrstuvwxyz"
                               "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                               "0123456789._+-/")
        != std::string::npos) {
        return false;
    }

    for (const std::filesystem::path &part : std::filesystem::path(path)) {
        if (part == "..") {
            return false;
        }
    }

    return true;
}

bool present(const std::string &path) {
    std::error_code error;

    return std::filesystem::exists(path, error);
}
}

InstallTask::InstallTask(Kernel::Version version, const Toolchain compiler, Kernel::Version replaced)
    : ProcessTask("Install task", 6),
      _version(std::move(version)),
      _compiler(compiler),
      _replaced(std::move(replaced)) {
}

// What a step creates afresh its undo removes, so a failed install leaves no half kernel
// behind. A step that writes over files already there has nothing to put back.
bool InstallTask::prepare() {
    const Settings *settings = Configuration::get();
    const std::string source = Kernel::get_source_directory(_version);
    const std::string version = _version.get_string();
    const std::string boot = settings->bootDirectory;

    if (!std::filesystem::exists(source)) {
        fail("Kernel directory " + source + " does not exist!");

        return false;
    }

    std::string image =
            Command("make -s --no-print-directory -C " + Shell::quote(source) + " image_name 2>/dev/null").capture();

    if (image.empty()) {
        image = "arch/x86/boot/bzImage";
    }

    if (!sane_relative_path(image)) {
        fail("The build names its image " + image + ", which is not a path this will install.");

        return false;
    }

    if (!std::filesystem::exists(source + "/" + image)) {
        fail("No kernel image at " + source + "/" + image + ", build the kernel first!");

        return false;
    }

    const std::string modules = "/usr/lib/modules/" + version;
    const std::string legacyModules = "/lib/modules/" + version;
    const bool freshModules = !present(modules) && !present(legacyModules);

    _steps.push_back(Step{.label = "Installing modules",
                          .command = Make::command("modules_install", _compiler),
                          .directory = source,
                          .elevated = true,
                          .optional = false,
                          .undo = freshModules ? "rm -rf " + Shell::quote(modules) + " " + Shell::quote(legacyModules)
                                               : std::string{}});

    const std::pair<std::string, std::string> copies[] = {
            {source + "/" + image, boot + "/vmlinuz-" + version},
            {source + "/System.map", boot + "/System.map-" + version},
            {source + "/.config", boot + "/config-" + version},
    };
    std::string install;
    std::string removal;

    for (const auto &[from, to] : copies) {
        install += (install.empty() ? "" : " && ") + std::string("install -Dvm644 ") + Shell::quote(from) + " "
                   + Shell::quote(to);

        if (!present(to)) {
            removal += " " + Shell::quote(to);
        }
    }

    _steps.push_back(Step{.label = "Installing image to " + boot + "/vmlinuz-" + version,
                          .command = install,
                          .directory = source,
                          .elevated = true,
                          .optional = false,
                          .undo = removal.empty() ? std::string{} : "rm -f" + removal});

    // Before the initramfs, since what dkms builds may have to go into it.
    if (Dkms::available()) {
        const std::string running = Kernel::get_current().get_version().get_string();
        const std::string replaced = _replaced.major > 0 ? _replaced.get_string() : std::string{};
        std::string carrying;

        if (!replaced.empty() && Dkms::present_for(replaced)) {
            carrying = replaced;
        } else if (Dkms::present_for(running)) {
            carrying = running;
        }

        report(carrying.empty() ? "No dkms modules are installed for " + (replaced.empty() ? running : replaced)
                                          + ", so none are required for " + version + "."
                                : carrying + " has dkms modules, so " + version + " must get them too.");

        _steps.push_back(Step{.label = "Installing dkms modules",
                              .command = "dkms autoinstall -k " + Shell::quote(version),
                              .directory = source,
                              .elevated = true,
                              .optional = carrying.empty(),
                              .undo = freshModules ? Dkms::removal(version) : std::string{}});
    }

    if (Command::exists("mkinitcpio")) {
        const std::string preset = "/etc/mkinitcpio.d/" + version + ".preset";
        const std::string initramfs = boot + "/initramfs-" + version + ".img";
        const std::string fallback = boot + "/initramfs-" + version + "-fallback.img";
        std::string images;

        for (const std::string &file : {initramfs, fallback}) {
            if (!present(file)) {
                images += " " + Shell::quote(file);
            }
        }

        _steps.push_back(Step{.label = "Creating mkinitcpio preset " + preset,
                              .command = "printf '%s\\n' " + Shell::quote("ALL_config=\"/etc/mkinitcpio.conf\"") + " "
                                         + Shell::quote("ALL_kver=\"" + boot + "/vmlinuz-" + version + "\"") + " "
                                         + Shell::quote("PRESETS=(\"default\" \"fallback\")") + " "
                                         + Shell::quote("default_image=\"" + initramfs + "\"") + " "
                                         + Shell::quote("fallback_image=\"" + fallback + "\"") + " "
                                         + Shell::quote("fallback_options=\"-S autodetect\"") + " > "
                                         + Shell::quote(preset),
                              .directory = source,
                              .elevated = true,
                              .optional = false,
                              .undo = present(preset) ? std::string{} : "rm -f " + Shell::quote(preset)});

        _steps.push_back(Step{.label = "Generating initramfs",
                              .command = "mkinitcpio -p " + Shell::quote(version),
                              .directory = source,
                              .elevated = true,
                              .optional = false,
                              .undo = images.empty() ? std::string{} : "rm -f" + images});
    } else if (Command::exists("dracut")) {
        const std::string initramfs = boot + "/initramfs-" + version + ".img";

        _steps.push_back(
                Step{.label = "Generating initramfs",
                     .command = "dracut --force --kver " + Shell::quote(version) + " " + Shell::quote(initramfs),
                     .directory = source,
                     .elevated = true,
                     .optional = false,
                     .undo = present(initramfs) ? std::string{} : "rm -f " + Shell::quote(initramfs)});
    } else {
        report("Neither mkinitcpio nor dracut is installed, skipping initramfs generation.");
    }

    // A grub.cfg means grub boots this machine, and then it has to learn of the new kernel.
    if (Command::exists("grub-mkconfig")) {
        const std::string config = settings->grubDirectory + "/grub.cfg";

        if (present(config)) {
            _steps.push_back(Step{.label = "Refreshing grub",
                                  .command = "grub-mkconfig -o " + Shell::quote(config),
                                  .directory = source,
                                  .elevated = true,
                                  .optional = false,
                                  .undo = {}});
        } else {
            report("No " + config + ", so grub is not refreshed.");
        }
    }

    return true;
}
