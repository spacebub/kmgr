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
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

// A pty gives the child a controlling terminal, which make, kconfig and sudo need to prompt.
class Process {
public:
    struct Prompt {
        std::string text;
        bool secret;
    };

private:
    std::string _command;
    std::string _workingDirectory;
    std::vector<std::string> _environment;

    std::atomic<pid_t> _pid = -1;
    std::atomic<int> _master = -1;
    std::atomic<bool> _canceled = false;
    std::mutex _writeMutex;

    std::string _line;
    bool _prompted = false;

    std::function<void (const std::string &data)> _onOutput = nullptr;
    std::function<void (const Prompt &prompt)> _onPrompt = nullptr;

    void consume(const char *data, size_t size);
    void check_prompt();

public:
    static constexpr int CANCELED = -1;

    explicit Process(std::string command);

    Process &with_directory(const std::string &directory);
    Process &with_environment(const std::string &variable, const std::string &value);
    Process &elevated();

    void on_output(const std::function<void (const std::string &data)> &callback);
    void on_prompt(const std::function<void (const Prompt &prompt)> &callback);

    [[nodiscard]] const std::string &get_command() const;
    [[nodiscard]] bool is_running() const;

    int run();
    void write(const std::string &input);
    void cancel();
};


#endif //KERNELMGR_PROCESS_H
