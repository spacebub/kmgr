// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2024
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "Workflow.h"

#include <algorithm>

#include "core/Log.h"

Workflow::Workflow(const std::string &name) {
    _name = name;
}

Workflow::~Workflow() {
    cleanup();
}

std::string Workflow::get_name() {
    return _name;
}

std::string Workflow::get_kernel() const {
    return _kernel;
}

std::string Workflow::get_log_path() const {
    return _logPath;
}

std::string Workflow::get_operation() const {
    return _operation;
}

void Workflow::set_context(const std::string &kernel, const std::string &operation) {
    _kernel = kernel;
    _operation = operation;
}

Workflow::Status Workflow::get_status() const {
    return _status;
}

bool Workflow::is_empty() const {
    return _tasks.empty();
}

size_t Workflow::size() const {
    return _tasks.size();
}

bool Workflow::run() {
    set_status(Running);

    _logPath = Log::open(_kernel, _operation);
    Log::write(Log::Level::Info, _name);

    bool success = true;

    for (Task *task : _tasks)  {
        if (_canceled) {
            success = false;
            break;
        }

        _current = task;

        if (_on_task_changed) {
            _on_task_changed(task->get_name());
        }

        if (_on_task_complete) {
            task->counter()->on_end([this](const ProgressEndArgs &args) {
                if (!args.canceled) {
                    _on_task_complete("Done!");

                    return;
                }

                _on_task_complete(_canceled ? "Canceled!" : "Failed!");
            });
        }

        if (_on_progress) {
            task->counter()->on_progress(_on_progress);
        }

        task->on_exception([this](const std::string &message) {
            Log::write(Log::Level::Error, message);

            if (_on_exception) {
                _on_exception(message);
            }
        });

        task->on_data_received([this](const std::string &data) {
            Log::write(Log::Level::Output, data);

            if (_on_data_received) {
                _on_data_received(data);
            }
        });

        task->on_step_changed([this](const std::string &label) {
            Log::write(Log::Level::Step, label);

            if (_on_step_changed) {
                _on_step_changed(label);
            }
        });

        task->on_input_required(_on_input_required);
        task->on_step_finished(_on_step_finished);

        if (!task->run()) {
            success = false;

            if (!_force || _canceled) {
                break;
            }
        }
    }

    _current = nullptr;

    Log::write(Log::Level::Info, success ? "Done" : "Failed");
    Log::close();

    set_status(success ? Done : Failed);

    return success;
}

void Workflow::sort() {
    std::ranges::sort(_tasks, [](Task *a, Task *b) {
        return a->position() < b->position();
    });
}

void Workflow::queue(Task *task) {
    _tasks.push_back(task);
}

void Workflow::clear() {
    cleanup();
    _tasks.clear();
}

void Workflow::cancel() {
    _canceled = true;

    if (Task *task = _current) {
        task->cancel();
    }
}

void Workflow::poll() const {
    if (const Task *task = _current) {
        task->counter()->poll();
    }
}

void Workflow::provide_input(const std::string &input) const {
    if (Task *task = _current) {
        task->provide_input(input);
    }
}

bool Workflow::is_interactive() const {
    const Task *task = _current;

    return task && task->is_interactive();
}

void Workflow::set_force(const bool force) {
    _force = force;
}

void Workflow::set_status(const Status status) {
    _status = status;

    if (_on_status_changed) {
        _on_status_changed(status);
    }
}

void Workflow::cleanup() const {
    for (const Task *task : _tasks) {
        delete task;
    }
}
