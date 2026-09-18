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


#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

// Sent through gdbus, so nothing is linked for it. Without gdbus, a bus or a
// listener, the alert is silently dropped.
class DesktopAlert {
public:
    enum class Urgency {
        Normal,
        Critical
    };

    DesktopAlert();

    // Each one replaces the one before it: only the last is still true.
    void post(const std::string &title, const std::string &body, Urgency urgency = Urgency::Normal);
    void withdraw();

private:
    // Shared with the reply thread, so the reply has somewhere to land after this
    // is gone.
    struct Held {
        std::mutex guard;
        std::uint32_t id = 0;
    };

    std::shared_ptr<Held> _held = std::make_shared<Held>();
    bool _available = false;
};


#endif //KERNELMGR_GUI_SERVICES_DESKTOPALERT_H
