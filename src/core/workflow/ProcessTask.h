// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_PROCESSTASK_H
#define KERNELMGR_PROCESSTASK_H


#include <mutex>
#include <vector>

#include "Task.h"
#include "core/Process.h"

class ProcessTask : public Task {
public:
    struct Step {
        std::string label;
        std::string command;
        std::string directory;
        bool elevated = false;
        bool optional = false;
    };

protected:
    std::vector<Step> _steps;
    int _position;

    mutable std::mutex _processMutex;
    std::shared_ptr<Process> _process = nullptr;
    std::atomic<bool> _canceled = false;
    std::atomic<bool> _awaitingSecret = false;

    virtual bool prepare() {
        return true;
    }

    bool execute(const Step &step);

public:
    ProcessTask(std::string name, int position);

    bool run() override;
    int position() override;
    void cancel() override;
    void provide_input(const std::string &input) override;

    [[nodiscard]] bool is_interactive() const override {
        return true;
    }
};


#endif //KERNELMGR_PROCESSTASK_H
