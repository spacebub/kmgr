// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_INK_H
#define KERNELMGR_INK_H


#include <cstdlib>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>

#include "core/Configuration.h"

#define ERASE "\x1B[K"
#define RESET "\x1B[0m"
#define BOLD "\x1B[1m"
#define DIM "\x1B[2m"
#define RED "\x1B[31m"
#define GREEN "\x1B[32m"
#define YELLOW "\x1B[33m"
#define BLUE "\x1B[34m"
#define CYAN "\x1B[36m"

namespace Ink {
    inline bool interactive() {
        static const bool tty = isatty(STDOUT_FILENO) == 1;

        return tty;
    }

    inline bool colored() {
        static const bool on = interactive()
            && getenv("NO_COLOR") == nullptr
            && Configuration::get()->colors;

        return on;
    }

    inline std::string paint(const std::string &text, const char *color) {
        return colored() ? std::string(color) + text + RESET : text;
    }

    inline int columns() {
        winsize size{};

        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && size.ws_col > 20) {
            return size.ws_col;
        }

        return 80;
    }

    inline std::string elide(const std::string &text, const size_t room) {
        if (text.length() <= room || room < 12) {
            return text;
        }

        const size_t head = room * 2 / 5;
        const size_t tail = room - head - 1;

        return text.substr(0, head) + "…" + text.substr(text.length() - tail);
    }

    inline std::string pretty(const std::string &path) {
        const char *home = getenv("HOME");

        if (home == nullptr || *home == '\0') {
            return path;
        }

        const std::string prefix = std::string(home) + "/";

        return path.starts_with(prefix) ? "~/" + path.substr(prefix.length()) : path;
    }
}


#endif //KERNELMGR_INK_H
