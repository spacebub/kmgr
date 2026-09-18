// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2024
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "Configuration.h"
#include <filesystem>
#include "yyjson.h"
#include <unistd.h>
#include <pwd.h>
#include <format>

namespace {
    constexpr auto CONF_FILENAME = "config.json";
    constexpr auto CONF_BOOT_DIR = "bootDirectory";
    constexpr auto CONF_GRUB_DIR = "grubDirectory";
    constexpr auto CONF_ARCHIVE_FORMAT = "archiveFormat";
    constexpr auto CONF_KERNEL_CDN = "kernelCdn";
    constexpr auto CONF_BASE_DIR = "baseDirectory";
    constexpr auto CONF_ARCHIVE_DIR = "archiveDirectory";
    constexpr auto CONF_ELEVATION = "elevationCommand";
    constexpr auto CONF_JOBS = "jobs";
    constexpr auto CONF_COLORS = "colors";
    constexpr auto CONF_COMPILER = "compiler";
    constexpr auto CONF_TOOLCHAIN_NAME = "customToolchainName";
    constexpr auto CONF_CUSTOM_FLAGS = "customFlags";
    constexpr auto CONF_THEME = "theme";
    constexpr auto CONF_NOTIFICATIONS = "notifications";

    std::unique_ptr<Settings> s_config;
    std::string s_homeDirectory;
    std::string s_configPath;
    std::string s_dataDirectory;

    std::string get_home_directory() {
        if (!s_homeDirectory.empty()) {
            return s_homeDirectory;
        }

        const char *home;

        if (!((home = getenv("HOME")))) {
            home = getpwuid(getuid())->pw_dir;
        }

        s_homeDirectory = home;
        return s_homeDirectory;
    }

    std::string get_config_path() {
        if (!s_configPath.empty()) {
            return s_configPath;
        }

        std::filesystem::path config;
        const char *xdgConfig = getenv("XDG_CONFIG_HOME");

        config.append(xdgConfig ? xdgConfig : get_home_directory() + "/.config");
        config.append("kernelmanager");
        config.append(CONF_FILENAME);

        s_configPath = config;
        return s_configPath;
    }

    std::string get_data_directory() {
        if (!s_dataDirectory.empty()) {
            return s_dataDirectory;
        }

        std::filesystem::path data;
        const char *xdgData = getenv("XDG_DATA_HOME");

        data.append(xdgData ? xdgData : get_home_directory() + "/.local/share");
        data.append("kernelmanager");

        s_dataDirectory = data;
        return s_dataDirectory;
    }

    void set_defaults() {
        const std::string base = get_data_directory();

        s_config->bootDirectory = "/boot";
        s_config->grubDirectory = "/boot/grub";
        s_config->archiveFormat = "tar.xz";
        s_config->kernelCdn = "https://cdn.kernel.org/pub/linux/kernel";
        s_config->baseDirectory = base;
        s_config->archiveDirectory = base + "/archives";
        s_config->elevationCommand = "sudo";
        s_config->jobs = 0;
        s_config->colors = true;
        s_config->compiler = Toolchain::Gcc;
        s_config->customToolchainName = "Custom";
        s_config->customFlags = {};
        s_config->theme = "system";
    }

    void read_value(yyjson_val *root, const char *key, std::string &target) {
        if (yyjson_val *field = yyjson_obj_get(root, key); field && yyjson_is_str(field)) {
            target = yyjson_get_str(field);
        }
    }

    void read_value(yyjson_val *root, const char *key, int &target) {
        if (yyjson_val *field = yyjson_obj_get(root, key); field && yyjson_is_int(field)) {
            target = yyjson_get_int(field);
        }
    }

    void read_value(yyjson_val *root, const char *key, bool &target) {
        if (yyjson_val *field = yyjson_obj_get(root, key); field && yyjson_is_bool(field)) {
            target = yyjson_get_bool(field);
        }
    }

    void read_compiler(yyjson_val *root, Toolchain &target) {
        if (yyjson_val *field = yyjson_obj_get(root, CONF_COMPILER); field && yyjson_is_str(field)) {
            if (const Toolchain named = toolchain_from(yyjson_get_str(field));
                    named != Toolchain::Unknown) {
                target = named;
            }
        }
    }

    void read_value(yyjson_val *root, const char *key, std::optional<bool> &target) {
        if (yyjson_val *field = yyjson_obj_get(root, key); field && yyjson_is_bool(field)) {
            target = yyjson_get_bool(field);
        }
    }
}

namespace Configuration {
    static void load() {
        constexpr yyjson_read_flag flags = YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS;
        yyjson_read_err err;
        yyjson_doc *doc = yyjson_read_file(get_config_path().c_str(), flags, nullptr, &err);

        // No file means the defaults, but a file that cannot be read is an error.
        if (err.code != YYJSON_READ_SUCCESS) {
            if (std::filesystem::exists(get_config_path())) {
                throw std::runtime_error(err.msg);
            }

            s_config = std::make_unique<Settings>();
            set_defaults();

            return;
        }

        s_config = std::make_unique<Settings>();
        set_defaults();

        yyjson_val *root = yyjson_doc_get_root(doc);

        read_value(root, CONF_BOOT_DIR, s_config->bootDirectory);
        read_value(root, CONF_GRUB_DIR, s_config->grubDirectory);
        read_value(root, CONF_ARCHIVE_FORMAT, s_config->archiveFormat);
        read_value(root, CONF_KERNEL_CDN, s_config->kernelCdn);
        read_value(root, CONF_BASE_DIR, s_config->baseDirectory);
        read_value(root, CONF_ARCHIVE_DIR, s_config->archiveDirectory);
        read_value(root, CONF_ELEVATION, s_config->elevationCommand);
        read_value(root, CONF_JOBS, s_config->jobs);
        read_value(root, CONF_COLORS, s_config->colors);
        read_compiler(root, s_config->compiler);
        read_value(root, CONF_TOOLCHAIN_NAME, s_config->customToolchainName);
        read_value(root, CONF_CUSTOM_FLAGS, s_config->customFlags);
        read_value(root, CONF_THEME, s_config->theme);
        read_value(root, CONF_NOTIFICATIONS, s_config->notifications);

        yyjson_doc_free(doc);
    }

    // The only writer, so the only place that creates the configured directories.
    void save() {
        std::error_code error;

        std::filesystem::create_directories(
            std::filesystem::path(get_config_path()).parent_path(), error);
        std::filesystem::create_directories(s_config->baseDirectory, error);
        std::filesystem::create_directories(s_config->archiveDirectory, error);

        yyjson_mut_doc *doc = yyjson_mut_doc_new(nullptr);
        yyjson_mut_val *root = yyjson_mut_obj(doc);
        yyjson_mut_doc_set_root(doc, root);

        // Kept alive here: the writer holds the pointer until the file is written.
        const std::string compiler = toolchain_name(s_config->compiler);

        yyjson_mut_obj_add_str(doc, root, CONF_BOOT_DIR, s_config->bootDirectory.c_str());
        yyjson_mut_obj_add_str(doc, root, CONF_GRUB_DIR, s_config->grubDirectory.c_str());
        yyjson_mut_obj_add_str(doc, root, CONF_ARCHIVE_FORMAT, s_config->archiveFormat.c_str());
        yyjson_mut_obj_add_str(doc, root, CONF_KERNEL_CDN, s_config->kernelCdn.c_str());
        yyjson_mut_obj_add_str(doc, root, CONF_BASE_DIR, s_config->baseDirectory.c_str());
        yyjson_mut_obj_add_str(doc, root, CONF_ARCHIVE_DIR, s_config->archiveDirectory.c_str());
        yyjson_mut_obj_add_str(doc, root, CONF_ELEVATION, s_config->elevationCommand.c_str());
        yyjson_mut_obj_add_int(doc, root, CONF_JOBS, s_config->jobs);
        yyjson_mut_obj_add_bool(doc, root, CONF_COLORS, s_config->colors);
        yyjson_mut_obj_add_str(doc, root, CONF_COMPILER, compiler.c_str());
        yyjson_mut_obj_add_str(doc, root, CONF_TOOLCHAIN_NAME, s_config->customToolchainName.c_str());
        yyjson_mut_obj_add_str(doc, root, CONF_CUSTOM_FLAGS, s_config->customFlags.c_str());
        yyjson_mut_obj_add_str(doc, root, CONF_THEME, s_config->theme.c_str());

        if (s_config->notifications.has_value()) {
            yyjson_mut_obj_add_bool(doc, root, CONF_NOTIFICATIONS, *s_config->notifications);
        }

        constexpr yyjson_write_flag flags = YYJSON_WRITE_PRETTY_TWO_SPACES;
        yyjson_write_err err;
        yyjson_mut_write_file(get_config_path().c_str(), doc, flags, nullptr, &err);

        yyjson_mut_doc_free(doc);

        if (err.code != 0) {
            throw std::runtime_error(err.msg);
        }
    }

    const Settings* get() {
        if (!s_config) {
            load();
        }

        return s_config.get();
    }

    void set(const Settings &settings) {
        if (!s_config) {
            return;
        }

        if (std::filesystem::exists(settings.bootDirectory)) {
            s_config->bootDirectory = settings.bootDirectory;
        }
        if (std::filesystem::exists(settings.grubDirectory)) {
            s_config->grubDirectory = settings.grubDirectory;
        }
        if (!settings.archiveFormat.empty()) {
            s_config->archiveFormat = settings.archiveFormat;
        }
        if (!settings.kernelCdn.empty()) {
            s_config->kernelCdn = settings.kernelCdn;
        }
        if (std::filesystem::exists(settings.baseDirectory)) {
            s_config->baseDirectory = settings.baseDirectory;
        }
        if (std::filesystem::exists(settings.archiveDirectory)) {
            s_config->archiveDirectory = settings.archiveDirectory;
        }
        if (!settings.elevationCommand.empty()) {
            s_config->elevationCommand = settings.elevationCommand;
        }
        if (settings.jobs >= 0) {
            s_config->jobs = settings.jobs;
        }

        s_config->colors = settings.colors;

        if (settings.compiler != Toolchain::Unknown) {
            s_config->compiler = settings.compiler;
        }

        if (!settings.customToolchainName.empty()) {
            s_config->customToolchainName = settings.customToolchainName;
        }
        // Taken as is: empty is a meaningful value and nothing else parses it.
        s_config->customFlags = settings.customFlags;

        // Never unset: a caller that leaves it out is not clearing it.
        if (settings.notifications.has_value()) {
            s_config->notifications = settings.notifications;
        }

        if (settings.theme == "system" || settings.theme == "light" || settings.theme == "dark") {
            s_config->theme = settings.theme;
        }
    }
}
