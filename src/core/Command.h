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


#include <string>
#include <functional>

class Command {
    std::string _command;

public:
    explicit Command(const std::string &command);

    [[nodiscard]] int execute() const;
    int execute(const std::function<void (const std::string &)> &callback) const;
    [[nodiscard]] std::string capture() const;

    static bool exists(const std::string &program);
};


#endif //KERNELMGR_COMMAND_H
