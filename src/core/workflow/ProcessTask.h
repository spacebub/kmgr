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
#include <cstdint>

class ProcessTask : public Task {
public:
    struct Step {
        std::string label;
        std::string command;
        std::string directory;
        bool elevated = false;
        bool optional = false;
        // Run, newest first, when the task fails or is canceled, so nothing is left half done.
        std::string undo;
    };

protected:
    std::vector<Step> _steps;
    int _position;

    mutable std::mutex _processMutex;
    std::shared_ptr<Process> _process = nullptr;
    std::atomic<bool> _canceled = false;
    std::atomic<bool> _awaitingSecret = false;

    virtual bool prepare() { return true; }

private:
    enum class Outcome : std::uint8_t {
        Done,
        Skipped,
        Failed,
        Canceled,
    };

    int launch(const std::string &command, const std::string &directory, bool elevated, bool evenIfCanceled);
    Outcome execute(const Step &step);
    void roll_back(const std::vector<const Step *> &attempted);

public:
    ProcessTask(std::string name, int position);

    bool run() override;
    int position() override;
    void cancel() override;
    void provide_input(const std::string &input) override;

    [[nodiscard]] bool is_interactive() const override { return true; }
};


#endif //KERNELMGR_PROCESSTASK_H
