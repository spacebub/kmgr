// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_PROCESS_H
#define KERNELMGR_PROCESS_H


#include <atomic>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>

// A pty gives the child a controlling terminal, which make, kconfig and sudo need to prompt.
class Process {
public:
    struct Prompt {
        std::string text;
        // The child turned echo off to read, so it is asking for something to keep quiet.
        bool secret;
        // The elevation command is still the one asking, nothing of ours has started yet.
        bool elevating;
    };

private:
    std::string _command;
    std::string _workingDirectory;
    bool _elevated = false;

    std::atomic<pid_t> _pid = -1;
    std::atomic<int> _master = -1;
    std::atomic<bool> _canceled = false;
    std::mutex _writeMutex;

    // Guards the line being read and what has been made of it.
    std::mutex _mutex;
    std::string _line;
    std::string _escape;
    bool _prompted = false;
    bool _elevating = false;

    std::function<void(const std::string &data)> _onOutput = nullptr;
    std::function<void(const Prompt &prompt)> _onPrompt = nullptr;

    std::string sanitize(const char *data, size_t size);
    void consume(const char *data, size_t size);
    void check_prompt();
    [[nodiscard]] bool reading_quietly() const;

public:
    static constexpr int CANCELED = -1;

    explicit Process(std::string command);

    Process &with_directory(const std::string &directory);
    Process &elevated();

    void on_output(const std::function<void(const std::string &data)> &callback);
    void on_prompt(const std::function<void(const Prompt &prompt)> &callback);

    [[nodiscard]] const std::string &get_command() const;
    [[nodiscard]] bool is_running() const;

    int run();
    void write(std::string_view input);
    void cancel();
};


#endif //KERNELMGR_PROCESS_H
