// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2024
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_COMMAND_H
#define KERNELMGR_COMMAND_H


#include <functional>
#include <string>

class Command {
    std::string _command;

public:
    explicit Command(std::string command);

    [[nodiscard]] int execute() const;
    int execute(const std::function<void(const std::string &)> &callback) const;

    // The output without its trailing newlines. The exit status comes back decoded, 128 and
    // up for a signal and -1 when the command could not be run at all.
    [[nodiscard]] std::string capture(int *status = nullptr) const;

    static bool exists(const std::string &program);
};


#endif //KERNELMGR_COMMAND_H
