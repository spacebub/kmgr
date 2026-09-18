// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <thread>
#include <vector>

#include "core/Command.h"
#include "core/Configuration.h"
#include "gui/services/DesktopAlert.h"
#include "ttk/system/Text.h"

namespace {
    constexpr auto SERVICE = "org.freedesktop.Notifications";
    constexpr auto PATH = "/org/freedesktop/Notifications";

    constexpr auto DEFAULT_TIMEOUT = "-1";
    constexpr auto UNTIL_DISMISSED = "0";

    std::string call(const std::string &method, const std::vector<std::string> &arguments) {
        std::string line = "gdbus call --session --dest " + std::string(SERVICE) + " --object-path "
            + PATH + " --method " + SERVICE + "." + method;

        for (const std::string &argument : arguments) {
            line += " " + ttk::Text::quote_argument(argument);
        }

        return line + " 2>/dev/null";
    }
}

DesktopAlert::DesktopAlert() : _available(Command::exists("gdbus")) {}

void DesktopAlert::post(const std::string &title, const std::string &body, const Urgency urgency) {
    if (!_available || !Configuration::get()->notifications.value_or(false)) {
        return;
    }

    std::uint32_t replaces = 0;

    {
        std::lock_guard lock(_held->guard);
        replaces = _held->id;
    }

    // The urgency has to reach the bus as a byte. The desktop takes the icon and
    // name from the desktop entry.
    const std::string hints = std::string("{'urgency': <byte ") + (urgency == Urgency::Critical ? "2" : "1")
        + ">, 'desktop-entry': <'kernelmgr'>}";

    const std::string line = call("Notify", {
        "KernelManager", std::to_string(replaces), "", title, body, "[]", hints,
        urgency == Urgency::Critical ? UNTIL_DISMISSED : DEFAULT_TIMEOUT
    });

    // The reply carries this alert's id, which the next one asks to replace.
    std::thread([line, held = _held] {
        std::string reply;

        try {
            reply = Command(line).capture();
        } catch (const std::exception &) {
            return;
        }

        const size_t at = reply.find("uint32 ");

        if (at == std::string::npos) {
            return;
        }

        std::lock_guard lock(held->guard);
        held->id = static_cast<std::uint32_t>(ttk::Text::to_int(
            ttk::Text::trim(reply.substr(at + 7, reply.find_first_of(",)", at) - at - 7))));
    }).detach();
}

void DesktopAlert::withdraw() {
    std::uint32_t id = 0;

    {
        std::lock_guard lock(_held->guard);
        id = _held->id;
        _held->id = 0;
    }

    if (!_available || id == 0) {
        return;
    }

    std::thread([line = call("CloseNotification", { std::to_string(id) })] {
        try {
            (void) Command(line).capture();
        } catch (const std::exception &) {
        }
    }).detach();
}
