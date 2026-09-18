// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2024
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_TASK_H
#define KERNELMGR_TASK_H


#include <string>
#include <functional>
#include <memory>

#include "core/Progress.h"

struct InputRequest {
    std::string prompt;
    bool secret;
};

class Task {
protected:
    std::string _name;
    std::function<void ()> _on_complete = nullptr;
    std::function<void (const std::string &message)> _on_exception = nullptr;
    std::function<void (const std::string &data)> _on_data_received = nullptr;
    std::function<void (const std::string &label)> _on_step_changed = nullptr;
    std::function<void (bool success)> _on_step_finished = nullptr;
    std::function<void (int progress)> _on_progress_changed = nullptr;
    std::function<void (const InputRequest &request)> _on_input_required = nullptr;
    std::unique_ptr<Progress> _progress = nullptr;

    void report(const std::string &message) const {
        if (_on_data_received) {
            _on_data_received(message + "\n");
        }
    }

    void step(const std::string &label) const {
        if (_on_step_changed) {
            _on_step_changed(label);

            return;
        }

        report("==> " + label);
    }

    void step_finished(const bool success) const {
        if (_on_step_finished) {
            _on_step_finished(success);
        }
    }

    void fail(const std::string &message) const {
        if (_on_exception) {
            _on_exception(message);

            return;
        }

        report(message);
    }

public:
    virtual ~Task() = default;

    explicit Task(std::string name, std::unique_ptr<Progress> counter) : _name(std::move(name)), _progress(std::move(counter)){}

    virtual bool run() = 0;
    virtual int position() = 0;

    virtual void cancel() {}
    virtual void provide_input(const std::string &) {}

    [[nodiscard]] virtual bool is_interactive() const {
        return false;
    }

    std::string get_name() {
        return _name;
    }

    [[nodiscard]] Progress *counter() const {
        return _progress.get();
    }

    void on_complete(const std::function<void ()> &callback) {
        _on_complete = callback;
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
};


#endif //KERNELMGR_TASK_H
