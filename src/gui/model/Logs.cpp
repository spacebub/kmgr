// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <filesystem>

#include "gui/app/Desk.h"
#include "gui/model/Format.h"
#include "gui/model/Logs.h"

std::string Logs::directory() {
    return Log::directory();
}

std::string Logs::size(const std::string &version) {
    const std::uintmax_t weight = Log::weigh(version);

    return weight == 0 ? std::string{} : Format::size(weight);
}

std::string Logs::build_size(const std::string &build) {
    const std::uintmax_t weight = Log::weigh(build, Log::Scope::Build);

    return weight == 0 ? std::string{} : Format::size(weight);
}

std::vector<Log::Filed> Logs::build_logs(const std::string &build) {
    return Log::filed(build);
}

std::vector<Log::Holding> Logs::all() {
    return Log::holdings();
}

void Logs::touch() {
    _revision++;

    if (changed) {
        changed();
    }
}

// No confirmation here: the caller has already asked.
bool Logs::remove(const std::string &version) {
    const std::uintmax_t before = Log::weigh(version);

    if (before == 0) {
        _notifier->info(version.empty()
            ? std::string("There are no logs to delete.")
            : "There are no logs for " + version + ".");

        return false;
    }

    const std::uintmax_t removed = Log::clear(version);

    touch();

    if (removed == 0) {
        _notifier->warning("Could not delete the logs in " + directory() + ".");

        return false;
    }

    _notifier->success("Deleted " + Format::size(removed) + " of logs"
        + (version.empty() ? "" : " for " + version) + ".");

    return true;
}

bool Logs::remove_build(const std::string &build) {
    const std::uintmax_t removed = Log::clear(build, Log::Scope::Build);

    touch();

    if (removed == 0) {
        _notifier->warning("Could not delete the logs for " + build + ".");

        return false;
    }

    _notifier->success("Deleted " + Format::size(removed) + " of logs for " + build + ".");

    return true;
}

bool Logs::remove_step(const std::string &build, const std::string &operation) {
    const std::uintmax_t removed = Log::clear_step(build, operation);

    touch();

    if (removed == 0) {
        _notifier->warning("Could not delete those logs for " + build + ".");

        return false;
    }

    _notifier->success("Deleted " + Format::size(removed) + " of logs for " + build + ".");

    return true;
}

// A build that has filed nothing has no directory yet, so the root opens
// instead. It is created first for a machine that has never run anything.
bool Logs::open(const std::string &build) const {
    const std::string own = Log::directory(build);
    const std::string target = std::filesystem::is_directory(own) ? own : Log::directory();
    std::error_code error;

    std::filesystem::create_directories(target, error);

    if (!Desk::reveal(target)) {
        _notifier->warning("Nothing on this system offered to open " + Desk::pretty(target) + ".");

        return false;
    }

    return true;
}
