// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <array>
#include <filesystem>
#include <poll.h>
#include <sys/inotify.h>
#include <unistd.h>

#include "gui/services/Watcher.h"

namespace {
    constexpr double SETTLE_SECONDS = 0.6;
}

Watcher::Watcher(ttk::Clock *clock, std::function<void()> settled)
        : _clock(clock), _settled(std::move(settled)) {
    _inotify = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);

    if (_inotify < 0 || pipe(_wake) != 0) {
        return;
    }

    _thread = std::thread([this] { listen(); });
}

Watcher::~Watcher() {
    if (_thread.joinable()) {
        (void) write(_wake[1], "x", 1);
        _thread.join();
    }

    for (const int fd : { _inotify, _wake[0], _wake[1] }) {
        if (fd >= 0) {
            close(fd);
        }
    }
}

// Watches are replaced under a running thread. inotify serialises the change,
// and a stale event only causes one more reread.
void Watcher::watch(const std::vector<std::string> &directories) {
    if (_inotify < 0) {
        return;
    }

    for (const int watch : _watches) {
        inotify_rm_watch(_inotify, watch);
    }

    _watches.clear();

    for (const std::string &directory : directories) {
        if (!std::filesystem::is_directory(directory)) {
            continue;
        }

        if (const int watch = inotify_add_watch(_inotify, directory.c_str(),
                                                IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_ATTRIB
                                                    | IN_DELETE_SELF | IN_MOVE_SELF);
            watch >= 0) {
            _watches.push_back(watch);
        }
    }
}

void Watcher::listen() {
    std::array<char, 4096> buffer{};

    while (true) {
        pollfd waiting[2] = {
            { .fd = _inotify, .events = POLLIN, .revents = 0 },
            { .fd = _wake[0], .events = POLLIN, .revents = 0 }
        };

        if (poll(waiting, 2, -1) < 0) {
            continue;
        }

        if (waiting[1].revents != 0) {
            return;
        }

        if (waiting[0].revents == 0) {
            continue;
        }

        while (read(_inotify, buffer.data(), buffer.size()) > 0) {
        }

        _clock->post([this] { stir(); });
    }
}

void Watcher::stir() {
    if (_alarm != 0) {
        _clock->cancel(_alarm);
    }

    _alarm = _clock->after(SETTLE_SECONDS, [this] {
        _alarm = 0;

        if (_settled) {
            _settled();
        }
    });
}
