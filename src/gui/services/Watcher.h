// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_SERVICES_WATCHER_H
#define KERNELMGR_GUI_SERVICES_WATCHER_H


#include <functional>
#include <string>
#include <thread>
#include <vector>

#include "ttk/util/Clock.h"

// Reports only once changes have been quiet for a moment, since an extraction
// touches thousands of files.
class Watcher {
public:
    Watcher(ttk::Clock *clock, std::function<void()> settled);
    ~Watcher();

    Watcher(const Watcher &) = delete;
    Watcher &operator=(const Watcher &) = delete;
    Watcher(Watcher &&) = delete;
    Watcher &operator=(Watcher &&) = delete;

    // Directories that are not there are skipped.
    void watch(const std::vector<std::string> &directories);

private:
    void listen();
    void stir();

    ttk::Clock *_clock;
    std::function<void()> _settled;

    int _inotify = -1;
    int _wake[2] = { -1, -1 };
    std::vector<int> _watches;
    int _alarm = 0;

    std::thread _thread;
};


#endif //KERNELMGR_GUI_SERVICES_WATCHER_H
