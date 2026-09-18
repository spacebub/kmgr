// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <cstring>
#include <poll.h>
#include <pty.h>
#include <csignal>
#include <stdexcept>
#include <sys/wait.h>
#include <unistd.h>

#include "Process.h"
#include "Configuration.h"

namespace {
    constexpr int POLL_INTERVAL = 100;
    constexpr int PROMPT_IDLE_INTERVAL = 600;
    constexpr size_t BUFFER_SIZE = 8192;
    constexpr size_t MAX_LINE = 4096;

    bool is_secret(const std::string &line) {
        std::string lowered = line;
        std::ranges::transform(lowered, lowered.begin(), tolower);

        return lowered.find("password") != std::string::npos
            || lowered.find("passphrase") != std::string::npos;
    }

    // Escape sequences are of no use to a log view and would break prompt matching.
    std::string sanitize(const char *data, const size_t size) {
        std::string clean;
        clean.reserve(size);

        for (size_t index = 0; index < size; ++index) {
            const char character = data[index];

            if (character == '\x1B') {
                size_t next = index + 1;

                if (next < size && (data[next] == '[' || data[next] == ']')) {
                    const bool osc = data[next] == ']';

                    for (++next; next < size; ++next) {
                        if (osc ? data[next] == '\a' : data[next] >= '@' && data[next] <= '~') {
                            break;
                        }
                    }
                }

                index = next;
                continue;
            }

            if (character == '\r' || character == '\b') {
                continue;
            }

            clean += character;
        }

        return clean;
    }
}

Process::Process(std::string command) : _command(std::move(command)) {
}

Process &Process::with_directory(const std::string &directory) {
    _workingDirectory = directory;

    return *this;
}

Process &Process::with_environment(const std::string &variable, const std::string &value) {
    _environment.push_back(variable + "=" + value);

    return *this;
}

Process &Process::elevated() {
    if (geteuid() == 0) {
        return *this;
    }

    std::string quoted = "'";

    for (const char character : _command) {
        if (character == '\'') {
            quoted += "'\\''";
        } else {
            quoted += character;
        }
    }

    quoted += '\'';
    _command = Configuration::get()->elevationCommand + " sh -c " + quoted;

    return *this;
}

void Process::on_output(const std::function<void(const std::string &)> &callback) {
    _onOutput = callback;
}

void Process::on_prompt(const std::function<void(const Prompt &)> &callback) {
    _onPrompt = callback;
}

const std::string &Process::get_command() const {
    return _command;
}

bool Process::is_running() const {
    return _pid > 0;
}

int Process::run() {
    std::vector<std::string> variables;

    for (char **entry = environ; *entry; ++entry) {
        variables.emplace_back(*entry);
    }

    variables.emplace_back("TERM=dumb");
    variables.emplace_back("NO_COLOR=1");
    variables.insert(variables.end(), _environment.begin(), _environment.end());

    std::vector<char *> envp;
    envp.reserve(variables.size() + 1);

    for (std::string &variable : variables) {
        envp.push_back(variable.data());
    }

    envp.push_back(nullptr);

    char shell[] = "/bin/sh";
    char flag[] = "-c";
    std::string command = _command;
    char *const argv[] = { shell, flag, command.data(), nullptr };

    constexpr winsize size {
        .ws_row = 50,
        .ws_col = 200,
        .ws_xpixel = 0,
        .ws_ypixel = 0
    };

    int master = -1;
    const pid_t pid = forkpty(&master, nullptr, nullptr, &size);

    if (pid < 0) {
        throw std::runtime_error("Could not start " + _command + ": " + strerror(errno));
    }

    if (pid == 0) {
        if (!_workingDirectory.empty() && chdir(_workingDirectory.c_str()) != 0) {
            _exit(127);
        }

        execve(shell, argv, envp.data());
        _exit(127);
    }

    _pid = pid;
    _master = master;
    _line.clear();
    _prompted = false;

    char buffer[BUFFER_SIZE];
    pollfd descriptor { .fd = master, .events = POLLIN, .revents = 0 };
    int idle = 0;
    bool killed = false;

    for (;;) {
        const int ready = poll(&descriptor, 1, POLL_INTERVAL);

        if (_canceled && !killed) {
            kill(-pid, SIGTERM);
            killed = true;
        }

        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }

            break;
        }

        if (ready == 0) {
            idle += POLL_INTERVAL;

            if (idle >= PROMPT_IDLE_INTERVAL) {
                check_prompt();
            }

            continue;
        }

        const ssize_t count = read(master, buffer, BUFFER_SIZE);

        if (count <= 0) {
            break;
        }

        idle = 0;
        consume(buffer, count);
    }

    int status = 0;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
    }

    _pid = -1;
    _master = -1;
    close(master);

    if (_canceled) {
        return CANCELED;
    }

    return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
}

void Process::write(const std::string &input) {
    std::lock_guard lock(_writeMutex);
    const int master = _master;

    if (master < 0) {
        return;
    }

    const std::string line = input + "\n";

    if (::write(master, line.c_str(), line.length()) < 0) {
        return;
    }

    _line.clear();
    _prompted = false;
}

void Process::cancel() {
    _canceled = true;

    const pid_t pid = _pid;

    if (pid > 0) {
        kill(-pid, SIGTERM);
    }
}

void Process::consume(const char *data, const size_t size) {
    const std::string clean = sanitize(data, size);

    if (clean.empty()) {
        return;
    }

    if (const size_t separator = clean.find_last_of('\n'); separator == std::string::npos) {
        if (_line.length() < MAX_LINE) {
            _line += clean;
        }
    } else {
        _line = clean.substr(separator + 1);
        _prompted = false;
    }

    if (_onOutput) {
        _onOutput(clean);
    }
}

void Process::check_prompt() {
    if (_prompted || _line.empty() || !_onPrompt || _canceled) {
        return;
    }

    std::string prompt = _line;
    prompt.erase(0, prompt.find_first_not_of(" \t"));

    if (prompt.empty()) {
        return;
    }

    _prompted = true;
    _onPrompt(Prompt { .text = prompt, .secret = is_secret(prompt) });
}
