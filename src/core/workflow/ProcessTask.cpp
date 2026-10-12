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
    std::vector<const Step *> attempted;

    try {
        success = prepare();

        for (const Step &step : _steps) {
            if (!success) {
                break;
            }

            if (_canceled) {
                success = false;
                break;
            }

            attempted.push_back(&step);

            const Outcome outcome = execute(step);

            success = outcome == Outcome::Done || outcome == Outcome::Skipped;
        }
    } catch (const std::exception &ex) {
        success = false;
        fail(ex.what());
    }

    if (!success) {
        if (_canceled) {
            report("Canceled.");
        }

        roll_back(attempted);
    }

    _awaitingSecret = false;
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

    std::shared_ptr<Process> process;

    {
        std::lock_guard lock(_processMutex);
        process = _process;
    }

    if (process) {
        process->write(input);
    }
}

int ProcessTask::launch(const std::string &command,
                        const std::string &directory,
                        const bool elevated,
                        const bool evenIfCanceled) {
    const auto process = std::make_shared<Process>(command);

    process->with_directory(directory);

    if (elevated) {
        process->elevated();
    }

    process->on_output([this](const std::string &data) {
        if (_on_data_received) {
            _on_data_received(data);
        }
    });

    // The password is only ever offered to the elevation command, and only once. A second
    // ask means the last answer was wrong, whoever gave it, so it is dropped and the
    // question goes through. What the command itself asks is never answered for it.
    auto answered = std::make_shared<std::atomic<bool>>(false);

    // The process owns this callback, so a shared pointer here would be a cycle that never frees.
    process->on_prompt([this, raw = process.get(), answered](const Process::Prompt &prompt) {
        bool remember = false;

        if (prompt.elevating && prompt.secret) {
            if (answered->exchange(true)) {
                Elevation::forget();
            } else if (Elevation::feed([raw](const std::string_view secret) { raw->write(secret); })) {
                return;
            }

            remember = true;
        }

        _awaitingSecret = remember;

        if (_on_input_required) {
            _on_input_required(InputRequest{.prompt = prompt.text, .secret = prompt.secret});
        }
    });

    {
        std::lock_guard lock(_processMutex);

        if (_canceled && !evenIfCanceled) {
            return Process::CANCELED;
        }

        _process = process;
    }

    const int status = process->run();

    {
        std::lock_guard lock(_processMutex);
        _process = nullptr;
    }

    _awaitingSecret = false;

    return status;
}

ProcessTask::Outcome ProcessTask::execute(const Step &step) {
    if (!step.label.empty()) {
        this->step(step.label);
    }

    const int status = launch(step.command, step.directory, step.elevated, false);

    step_finished(status == 0);

    if (status == 0) {
        return Outcome::Done;
    }

    if (status == Process::CANCELED) {
        return Outcome::Canceled;
    }

    const std::string message =
            (step.label.empty() ? step.command : step.label) + " failed with status " + std::to_string(status);

    if (step.optional) {
        report("(skipped) " + message);

        return Outcome::Skipped;
    }

    fail(message);

    return Outcome::Failed;
}

// The step that failed is taken back too, since it may have got part of the way.
void ProcessTask::roll_back(const std::vector<const Step *> &attempted) {
    std::vector<const Step *> undoable;

    for (const Step *step : attempted) {
        if (!step->undo.empty()) {
            undoable.push_back(step);
        }
    }

    if (undoable.empty()) {
        return;
    }

    step("Taking back what was done");

    bool clean = true;

    for (auto it = undoable.rbegin(); it != undoable.rend(); ++it) {
        const Step *step = *it;

        report("Undoing: " + step->label);

        try {
            if (launch(step->undo, step->directory, step->elevated, true) != 0) {
                clean = false;
            }
        } catch (const std::exception &ex) {
            report(ex.what());
            clean = false;
        }
    }

    step_finished(clean);

    if (!clean) {
        report("Not everything could be taken back, the log says what was tried.");
    }
}
