// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2024
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_WORKFLOW_H
#define KERNELMGR_WORKFLOW_H


#include <atomic>
#include <vector>
#include <string>
#include "Task.h"

class Workflow {
public:
    enum Status {
        Pending = 0,
        Running = 1,
        Failed = 2,
        Done = 3
    };

private:
    std::string _name;
    std::string _kernel;
    std::string _operation;
    std::string _logPath;
    std::vector<Task *> _tasks;
    std::atomic<Task *> _current = nullptr;
    std::atomic<bool> _canceled = false;
    bool _force = false;
    Status _status = Pending;

    std::function<void (const Status &)> _on_status_changed = nullptr;
    std::function<void (const std::string &)> _on_task_changed = nullptr;
    std::function<void (const std::string &)> _on_task_complete = nullptr;
    std::function<void (const std::string &)> _on_exception = nullptr;
    std::function<void (const std::string &)> _on_data_received = nullptr;
    std::function<void (const std::string &)> _on_step_changed = nullptr;
    std::function<void (bool)> _on_step_finished = nullptr;
    std::function<void (const InputRequest &)> _on_input_required = nullptr;
    std::function<void (const ProgressArgs &)> _on_progress = nullptr;

    void cleanup() const;
    void set_status(Status status);

public:
    explicit Workflow(const std::string &name);
    ~Workflow();

    std::string get_name();
    [[nodiscard]] std::string get_kernel() const;
    [[nodiscard]] std::string get_log_path() const;
    [[nodiscard]] std::string get_operation() const;
    void set_context(const std::string &kernel, const std::string &operation);
    [[nodiscard]] Status get_status() const;
    [[nodiscard]] bool is_empty() const;
    [[nodiscard]] size_t size() const;

    bool run();
    void sort();
    void queue(Task *task);
    void clear();

    void cancel();
    void poll() const;
    void provide_input(const std::string &input) const;
    [[nodiscard]] bool is_interactive() const;

    void set_force(bool force);

    void on_status_changed(const std::function<void (const Status &)> &callback) {
        _on_status_changed = callback;
    }

    void on_task_changed(const std::function<void (const std::string &)> &callback) {
        _on_task_changed = callback;
    }

    void on_task_complete(const std::function<void (const std::string &)> &callback) {
        _on_task_complete = callback;
    }

    void on_exception(const std::function<void (const std::string &message)> &callback) {
        _on_exception = callback;
    }

    void on_data_received(const std::function<void (const std::string &data)> &callback) {
        _on_data_received = callback;
    }

    void on_step_changed(const std::function<void (const std::string &label)> &callback) {
        _on_step_changed = callback;
    }

    void on_step_finished(const std::function<void (bool success)> &callback) {
        _on_step_finished = callback;
    }

    void on_input_required(const std::function<void (const InputRequest &request)> &callback) {
        _on_input_required = callback;
    }

    void on_progress(const std::function<void (const ProgressArgs &args)> &callback) {
        _on_progress = callback;
    }
};


#endif //KERNELMGR_WORKFLOW_H
