// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <mutex>

#include "Elevation.h"

namespace {
    std::mutex s_mutex;
    std::optional<std::string> s_secret;
}

void Elevation::remember(const std::string &secret) {
    if (secret.empty()) {
        return;
    }

    std::lock_guard lock(s_mutex);
    s_secret = secret;
}

std::optional<std::string> Elevation::recall() {
    std::lock_guard lock(s_mutex);

    return s_secret;
}

void Elevation::forget() {
    std::lock_guard lock(s_mutex);

    if (s_secret) {
        // Overwritten rather than dropped, so it does not linger in freed memory.
        s_secret->assign(s_secret->size(), '\0');
    }

    s_secret.reset();
}

bool Elevation::remembered() {
    std::lock_guard lock(s_mutex);

    return s_secret.has_value();
}
