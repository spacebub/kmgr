// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <sys/mman.h>
#include <sys/prctl.h>

#include "Elevation.h"

namespace {
constexpr size_t CAPACITY = 4096;

std::mutex s_mutex;
alignas(CAPACITY) char s_secret[CAPACITY];
size_t s_length = 0;
bool s_pinned = false;
int s_dumpable = 1;

// Also keeps other processes of the same user from reading this one through ptrace.
void pin() {
    if (s_pinned) {
        return;
    }

    s_pinned = true;

    mlock(s_secret, CAPACITY);
    madvise(s_secret, CAPACITY, MADV_DONTDUMP);

    s_dumpable = prctl(PR_GET_DUMPABLE, 0, 0, 0, 0);
    prctl(PR_SET_DUMPABLE, 0, 0, 0, 0);

    static_cast<void>(atexit([] { Elevation::forget(); }));
}

void unpin() {
    if (!s_pinned) {
        return;
    }

    s_pinned = false;

    prctl(PR_SET_DUMPABLE, s_dumpable < 0 ? 1 : s_dumpable, 0, 0, 0);
    munlock(s_secret, CAPACITY);
}

void clear() {
    explicit_bzero(s_secret, CAPACITY);
    s_length = 0;
}
}

void Elevation::remember(const std::string &secret) {
    if (secret.empty() || secret.size() >= CAPACITY) {
        return;
    }

    const std::scoped_lock lock(s_mutex);

    pin();
    clear();

    // NOLINTNEXTLINE(bugprone-not-null-terminated-result): the length is kept beside it, the buffer is no string.
    memcpy(s_secret, secret.data(), secret.size());
    s_length = secret.size();
}

bool Elevation::feed(const std::function<void(std::string_view)> &sink) {
    const std::scoped_lock lock(s_mutex);

    if (s_length == 0) {
        return false;
    }

    sink(std::string_view(s_secret, s_length));

    return true;
}

void Elevation::forget() {
    const std::scoped_lock lock(s_mutex);

    clear();
    unpin();
}

bool Elevation::remembered() {
    const std::scoped_lock lock(s_mutex);

    return s_length > 0;
}

void Elevation::wipe(std::string &text) {
    if (!text.empty()) {
        explicit_bzero(text.data(), text.size());
    }

    text.clear();
}
