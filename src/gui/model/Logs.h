// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_MODEL_LOGS_H
#define KERNELMGR_GUI_MODEL_LOGS_H


#include <functional>
#include <string>
#include <vector>

#include "core/Log.h"
#include "ttk/notices/Notifier.h"

// An empty version covers every log, including runs for no kernel in particular.
class Logs {
public:
    explicit Logs(ttk::Notifier *notifier) : _notifier(notifier) {}

    [[nodiscard]] static std::string directory();

    // Empty when nothing is filed, so the caller can word it.
    [[nodiscard]] static std::string size(const std::string &version);
    [[nodiscard]] static std::string build_size(const std::string &build);

    // Runs that were more than one step come back under an empty name.
    [[nodiscard]] static std::vector<Log::Filed> build_logs(const std::string &build);

    // Heaviest first.
    [[nodiscard]] static std::vector<Log::Holding> all();

    bool remove(const std::string &version);
    bool remove_build(const std::string &build);
    bool remove_step(const std::string &build, const std::string &operation);

    bool open(const std::string &build) const;

    void touch();

    [[nodiscard]] int revision() const { return _revision; }

    std::function<void()> changed;

private:
    ttk::Notifier *_notifier;
    int _revision = 0;
};


#endif //KERNELMGR_GUI_MODEL_LOGS_H
