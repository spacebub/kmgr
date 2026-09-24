// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <utility>

#include "core/Configuration.h"
#include "gui/services/DesktopAlert.h"

namespace {
    constexpr auto ICON = "kernelmgr";

    constexpr int DESKTOP_TIMEOUT = -1;
    constexpr int UNTIL_DISMISSED = 0;
}

DesktopAlert::DesktopAlert() : _worker([this](const std::stop_token &stop) { run(stop); }) {}

// The worker is joined before the bus it may still be using is dropped.
DesktopAlert::~DesktopAlert() {
    _worker.request_stop();
    _worker.join();

    ttk::Notify::stop();
}

void DesktopAlert::post(const std::string &title, const std::string &body, const Urgency urgency) {
    if (!Configuration::get()->notifications.value_or(false)) {
        return;
    }

    {
        const std::scoped_lock lock(_guard);

        _next = ttk::Notify::Message{
            .title = title,
            .body = body,
            .icon = ICON,
            .urgency = urgency,
            .timeout = urgency == Urgency::Critical ? UNTIL_DISMISSED : DESKTOP_TIMEOUT,
        };
        _withdraw = false;
    }

    _wake.notify_one();
}

void DesktopAlert::withdraw() {
    {
        const std::scoped_lock lock(_guard);

        _next.reset();
        _withdraw = true;
    }

    _wake.notify_one();
}

void DesktopAlert::run(const std::stop_token &stop) {
    while (true) {
        std::optional<ttk::Notify::Message> next;
        bool withdraw = false;

        {
            std::unique_lock lock(_guard);

            if (!_wake.wait(lock, stop, [this] { return _next.has_value() || _withdraw; })) {
                return;
            }

            next.swap(_next);
            withdraw = std::exchange(_withdraw, false);
        }

        if (next) {
            _shown = ttk::Notify::show(*next, _shown);
        } else if (withdraw) {
            ttk::Notify::close(std::exchange(_shown, 0));
        }
    }
}
