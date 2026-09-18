// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "ProcessTask.h"
#include "core/Elevation.h"

ProcessTask::ProcessTask(std::string name, const int position)
        : Task(std::move(name), std::make_unique<Progress>()), _position(position) {
}

bool ProcessTask::run() {
    _progress->set_total(0);

    bool success = true;

    try {
        success = prepare();

        for (const Step &step : _steps) {
            if (!success || _canceled) {
                break;
            }

            success = execute(step);
        }
    } catch (const std::exception &ex) {
        success = false;
        fail(ex.what());
    }

    if (!success && _canceled) {
        report("Canceled.");
    }

    _progress->end(!success);

    if (success && _on_complete) {
        _on_complete();
    }

    return success;
}

int ProcessTask::position() {
    return _position;
}

void ProcessTask::cancel() {
    _canceled = true;

    std::lock_guard lock(_processMutex);

    if (_process) {
        _process->cancel();
    }
}

void ProcessTask::provide_input(const std::string &input) {
    if (_awaitingSecret.exchange(false)) {
        Elevation::remember(input);
    }

    std::lock_guard lock(_processMutex);

    if (_process) {
        _process->write(input);
    }
}

bool ProcessTask::execute(const Step &step) {
    if (!step.label.empty()) {
        this->step(step.label);
    }

    const auto process = std::make_shared<Process>(step.command);

    process->with_directory(step.directory);

    if (step.elevated) {
        process->elevated();
    }

    process->on_output([this](const std::string &data) {
        if (_on_data_received) {
            _on_data_received(data);
        }
    });

    // A known password is offered once. A second prompt means it was wrong, so it is
    // dropped and the question goes through.
    auto offered = std::make_shared<std::atomic<bool>>(false);

    // The process owns this callback, so a shared pointer here would be a cycle that never frees.
    process->on_prompt([this, raw = process.get(), offered](const Process::Prompt &prompt) {
        if (prompt.secret) {
            if (!offered->exchange(true)) {
                if (const std::optional<std::string> secret = Elevation::recall()) {
                    raw->write(*secret);

                    return;
                }
            } else {
                Elevation::forget();
            }
        }

        _awaitingSecret = prompt.secret;

        if (_on_input_required) {
            _on_input_required(InputRequest { .prompt = prompt.text, .secret = prompt.secret });
        }
    });

    {
        std::lock_guard lock(_processMutex);

        if (_canceled) {
            return false;
        }

        _process = process;
    }

    const int status = process->run();

    {
        std::lock_guard lock(_processMutex);
        _process = nullptr;
    }

    step_finished(status == 0);

    if (status == 0) {
        return true;
    }

    if (status == Process::CANCELED) {
        return false;
    }

    const std::string message = (step.label.empty() ? step.command : step.label)
        + " failed with status " + std::to_string(status);

    if (step.optional) {
        report("(skipped) " + message);

        return true;
    }

    fail(message);

    return false;
}
