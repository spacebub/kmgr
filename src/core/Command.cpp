// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2024
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <stdexcept>
#include <sys/wait.h>
#include <utility>

#include "Command.h"
#include "Shell.h"

Command::Command(std::string command) : _command(std::move(command)) {
}

int Command::execute() const {
    // NOLINTNEXTLINE(cert-env33-c,bugprone-command-processor,concurrency-mt-unsafe): a shell line is what this runs.
    return system(_command.c_str());
}

std::string Command::capture(int *status) const {
    std::string output;
    const int raw = execute([&output](const std::string &data) { output += data; });

    if (status != nullptr) {
        *status = raw == -1 ? -1 : WIFEXITED(raw) ? WEXITSTATUS(raw) : 128 + WTERMSIG(raw);
    }

    while (!output.empty() && (output.back() == '\n' || output.back() == '\r')) {
        output.pop_back();
    }

    return output;
}

bool Command::exists(const std::string &program) {
    return Command("command -v " + Shell::quote(program) + " > /dev/null 2>&1").execute() == 0;
}

int Command::execute(const std::function<void(const std::string &)> &callback) const {
    if (!callback) {
        throw std::invalid_argument("Invalid callback!");
    }

    // NOLINTNEXTLINE(cert-env33-c,bugprone-command-processor): a shell line is what this runs.
    FILE *pipe = popen(_command.c_str(), "r");

    if (pipe == nullptr) {
        throw std::runtime_error("Could not run command " + _command);
    }

    char buffer[128];

    while (fgets(buffer, 128, pipe) != nullptr) {
        callback(buffer);
    }

    return pclose(pipe);
}
