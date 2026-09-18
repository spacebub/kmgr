// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <filesystem>

#include "core/Kernel.h"
#include "gui/app/Desk.h"
#include "ttk/util/Clipboard.h"
#include "ttk/util/Desktop.h"
#include "ttk/util/Format.h"

namespace Desk {

    bool reveal(const std::string &path) {
        return !path.empty() && ttk::Desktop::open(path);
    }

    std::string pretty(const std::string &path) {
        return ttk::Format::pretty_path(path);
    }

    void copy(const std::string &text) {
        ttk::Clipboard::write(text);
    }

    bool is_directory(const std::string &path) {
        std::error_code error;

        return std::filesystem::is_directory(path, error);
    }

    bool is_file(const std::string &path) {
        std::error_code error;

        return std::filesystem::is_regular_file(path, error);
    }

    std::string resolved_config(const std::string &version, const std::string &suffix) {
        return Kernel::find_config(Kernel::get_version(version, suffix));
    }

    std::string resolved_patch(const std::string &version, const std::string &suffix) {
        return Kernel::find_patch(Kernel::get_version(version, suffix));
    }

}
