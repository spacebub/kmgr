// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2024
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <stdexcept>
#include "Command.h"


Command::Command(const std::string &command) {
    _command = command;
}

int Command::execute() const {
    return system(_command.c_str());
}

std::string Command::capture() const {
    std::string output;

    execute([&output](const std::string &data) { output += data; });

    while (!output.empty() && (output.back() == '\n' || output.back() == '\r')) {
        output.pop_back();
    }

    return output;
}

bool Command::exists(const std::string &program) {
    return Command("command -v " + program + " > /dev/null 2>&1").execute() == 0;
}

int Command::execute(const std::function<void (const std::string &)> &callback) const {
    if (!callback) {
        throw std::invalid_argument("Invalid callback!");
    }

    FILE *pipe = popen(_command.c_str(), "r");

    if (!pipe) {
        throw std::runtime_error("Could not run command " + _command);
    }

    char buffer[128];

    while (fgets(buffer, 128, pipe) != nullptr) {
        callback(buffer);
    }

    return pclose(pipe);
}
