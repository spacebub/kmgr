// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_MODEL_FORMAT_H
#define KERNELMGR_GUI_MODEL_FORMAT_H


#include <cstdint>
#include <format>
#include <iterator>
#include <string>

namespace Format {
    inline std::string size(const std::uintmax_t bytes) {
        static constexpr const char *units[] = { "B", "KB", "MB", "GB" };
        double value = static_cast<double>(bytes);
        size_t unit = 0;

        while (value >= 1024.0 && unit + 1 < std::size(units)) {
            value /= 1024.0;
            ++unit;
        }

        return std::format("{:.1f} {}", value, units[unit]);
    }

    inline std::string runs(const int count) {
        return std::to_string(count) + (count == 1 ? " run" : " runs");
    }
}


#endif //KERNELMGR_GUI_MODEL_FORMAT_H
