// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_APP_DESK_H
#define KERNELMGR_GUI_APP_DESK_H


#include <string>

namespace Desk {
    bool reveal(const std::string &path);

    [[nodiscard]] std::string pretty(const std::string &path);

    void copy(const std::string &text);

    [[nodiscard]] bool is_directory(const std::string &path);
    [[nodiscard]] bool is_file(const std::string &path);

    [[nodiscard]] std::string resolved_config(const std::string &version, const std::string &suffix);
    [[nodiscard]] std::string resolved_patch(const std::string &version, const std::string &suffix);
}


#endif //KERNELMGR_GUI_APP_DESK_H
