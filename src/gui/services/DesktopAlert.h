// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_SERVICES_DESKTOPALERT_H
#define KERNELMGR_GUI_SERVICES_DESKTOPALERT_H


#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

#include "ttk/system/Notify.h"

// Without a bus or a notification server, the alert is silently dropped.
class DesktopAlert {
public:
    using Urgency = ttk::Notify::Urgency;

    DesktopAlert();
    ~DesktopAlert();

    DesktopAlert(const DesktopAlert &) = delete;
    DesktopAlert &operator=(const DesktopAlert &) = delete;
    DesktopAlert(DesktopAlert &&) = delete;
    DesktopAlert &operator=(DesktopAlert &&) = delete;

    // Each one replaces the one before it: only the last is still true.
    void post(const std::string &title, const std::string &body, Urgency urgency = Urgency::Normal);
    void withdraw();

private:
    // The desktop can take seconds to answer, so it is only ever asked from here.
    void run(const std::stop_token &stop);

    std::mutex _guard;
    std::condition_variable_any _wake;

    // What the desktop should show next. A request not yet sent is dropped by a later one.
    std::optional<ttk::Notify::Message> _next;
    bool _withdraw = false;

    ttk::Notify::Id _shown = 0;

    std::jthread _worker;
};


#endif //KERNELMGR_GUI_SERVICES_DESKTOPALERT_H
