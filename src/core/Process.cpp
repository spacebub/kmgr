// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <chrono>
#include <csignal>
#include <cstring>
#include <format>
#include <map>
#include <poll.h>
#include <pty.h>
#include <stdexcept>
#include <sys/uio.h>
#include <sys/wait.h>
#include <system_error>
#include <termios.h>
#include <unistd.h>
#include <vector>

#include "Configuration.h"
#include "Process.h"
#include "Shell.h"

namespace {
constexpr int POLL_INTERVAL = 100;
constexpr int PROMPT_IDLE_INTERVAL = 600;
constexpr int KILL_GRACE = 5000;
constexpr size_t BUFFER_SIZE = 8192;
constexpr size_t MAX_LINE = 4096;
constexpr size_t MAX_ESCAPE = 256;

// The elevated shell prints this before the command proper, as a terminal sequence nothing
// shows, so a prompt before it is the elevation command's and one after it is the command's.
constexpr auto STARTED = "kernelmgr";
}

Process::Process(std::string command) : _command(std::move(command)) {
}

Process &Process::with_directory(const std::string &directory) {
    _workingDirectory = directory;

    return *this;
}

Process &Process::elevated() {
    if (geteuid() == 0) {
        return *this;
    }

    _elevated = true;
    _command = Configuration::get()->elevationCommand + " sh -c "
               + Shell::quote(std::string("printf '\\033]") + STARTED + "\\a'; " + _command);

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
    // The child looks a name up from the front, so an override has to replace, not follow.
    std::map<std::string, std::string> variables;

    for (char *const *entry = environ; *entry != nullptr; ++entry) {
        const std::string_view pair(*entry);
        const size_t equals = pair.find('=');

        if (equals != std::string_view::npos) {
            variables[std::string(pair.substr(0, equals))] = std::string(pair.substr(equals + 1));
        }
    }

    variables["TERM"] = "dumb";
    variables["NO_COLOR"] = "1";

    std::vector<std::string> flattened;
    flattened.reserve(variables.size());

    for (const auto &[name, value] : variables) {
        flattened.push_back(std::format("{}={}", name, value));
    }

    std::vector<char *> envp;
    envp.reserve(flattened.size() + 1);

    for (std::string &variable : flattened) {
        envp.push_back(variable.data());
    }

    envp.push_back(nullptr);

    char shell[] = "/bin/sh";
    char flag[] = "-c";
    std::string command = _command;
    char *const argv[] = {shell, flag, command.data(), nullptr};

    constexpr winsize size{.ws_row = 50, .ws_col = 200, .ws_xpixel = 0, .ws_ypixel = 0};

    int master = -1;
    const pid_t pid = forkpty(&master, nullptr, nullptr, &size);

    if (pid < 0) {
        throw std::runtime_error("Could not start " + _command + ": " + std::generic_category().message(errno));
    }

    if (pid == 0) {
        if (!_workingDirectory.empty() && chdir(_workingDirectory.c_str()) != 0) {
            _exit(127);
        }

        execve(shell, argv, envp.data());
        _exit(127);
    }

    {
        const std::scoped_lock lock(_mutex);

        _line.clear();
        _escape.clear();
        _prompted = false;
        _elevating = _elevated;
    }

    _pid = pid;
    _master = master;

    char buffer[BUFFER_SIZE];
    pollfd descriptor{.fd = master, .events = POLLIN, .revents = 0};
    int idle = 0;
    bool killed = false;
    bool forced = false;
    std::chrono::steady_clock::time_point killedAt;

    for (;;) {
        const int ready = poll(&descriptor, 1, POLL_INTERVAL);

        if (_canceled && !killed) {
            kill(-pid, SIGTERM);
            killed = true;
            killedAt = std::chrono::steady_clock::now();
        } else if (killed && !forced
                   && std::chrono::steady_clock::now() - killedAt > std::chrono::milliseconds(KILL_GRACE)) {
            kill(-pid, SIGKILL);
            forced = true;
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

// Handed over in one call and never joined with the newline, so a secret is not copied here.
void Process::write(const std::string_view input) {
    char newline = '\n';
    const iovec parts[] = {
            {.iov_base = const_cast<char *>(input.data()), .iov_len = input.size()},
            {.iov_base = &newline, .iov_len = 1},
    };

    {
        const std::scoped_lock lock(_writeMutex);
        const int master = _master;

        if (master < 0 || writev(master, parts, 2) < 0) {
            return;
        }
    }

    const std::scoped_lock lock(_mutex);

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

// Escape sequences are of no use to a log view and would break prompt matching. One cut
// off by the end of a read is kept back for the next. The caller holds the lock.
std::string Process::sanitize(const char *data, const size_t size) {
    const std::string raw = _escape + std::string(data, size);
    std::string clean;

    _escape.clear();
    clean.reserve(raw.size());

    for (size_t index = 0; index < raw.size(); ++index) {
        const char character = raw[index];

        if (character == '\x1B') {
            size_t next = index + 1;
            bool complete = next < raw.size();

            if (complete && (raw[next] == '[' || raw[next] == ']')) {
                const bool osc = raw[next] == ']';
                const size_t body = next + 1;

                complete = false;

                for (++next; next < raw.size(); ++next) {
                    if (osc ? raw[next] == '\a'
                                        || (raw[next] == '\x1B' && next + 1 < raw.size() && raw[next + 1] == '\\')
                            : raw[next] >= '@' && raw[next] <= '~') {
                        complete = true;
                        break;
                    }
                }

                if (complete && osc) {
                    if (raw.compare(body, next - body, STARTED) == 0) {
                        _elevating = false;
                    }

                    if (raw[next] == '\x1B') {
                        ++next;
                    }
                }
            }

            if (!complete) {
                if (raw.size() - index <= MAX_ESCAPE) {
                    _escape = raw.substr(index);
                }

                break;
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

void Process::consume(const char *data, const size_t size) {
    std::string clean;

    {
        const std::scoped_lock lock(_mutex);

        clean = sanitize(data, size);

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
    }

    if (_onOutput) {
        _onOutput(clean);
    }
}

// The master side reports the terminal settings the child made. Echo off in canonical mode
// is a program reading a line it does not want shown, as sudo, doas and ssh all do.
bool Process::reading_quietly() const {
    termios settings{};
    const int master = _master;

    return master >= 0 && tcgetattr(master, &settings) == 0 && (settings.c_lflag & ECHO) == 0
           && (settings.c_lflag & ICANON) != 0;
}

void Process::check_prompt() {
    if (!_onPrompt || _canceled) {
        return;
    }

    Prompt prompt;

    {
        const std::scoped_lock lock(_mutex);

        if (_prompted) {
            return;
        }

        const bool secret = reading_quietly();
        std::string text = _line;

        text.erase(0, text.find_first_not_of(" \t"));
        text.erase(text.find_last_not_of(" \t") + 1);

        if (text.empty() && !secret) {
            return;
        }

        _prompted = true;
        prompt = Prompt{.text = text, .secret = secret, .elevating = _elevating};
    }

    _onPrompt(prompt);
}
