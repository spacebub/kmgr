// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_MODEL_WORKFLOWRUNNER_H
#define KERNELMGR_GUI_MODEL_WORKFLOWRUNNER_H


#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "core/BuildConfig.h"
#include "core/workflow/WorkflowFactory.h"
#include "gui/services/DesktopAlert.h"
#include "ttk/notices/Notifier.h"
#include "ttk/util/Clock.h"

class WorkflowRunner {
public:
    WorkflowRunner(ttk::Clock *clock, ttk::Notifier *notifier, DesktopAlert *alert);
    ~WorkflowRunner();

    WorkflowRunner(const WorkflowRunner &) = delete;
    WorkflowRunner &operator=(const WorkflowRunner &) = delete;
    WorkflowRunner(WorkflowRunner &&) = delete;
    WorkflowRunner &operator=(WorkflowRunner &&) = delete;

    [[nodiscard]] bool active() const { return _running || _finished; }
    [[nodiscard]] bool running() const { return _running; }
    [[nodiscard]] bool finished() const { return _finished; }
    [[nodiscard]] bool succeeded() const { return _succeeded; }
    [[nodiscard]] bool canceling() const { return _canceling; }
    [[nodiscard]] const std::string &title() const { return _title; }
    [[nodiscard]] const std::string &task() const { return _task; }
    [[nodiscard]] int progress() const { return _progress; }
    [[nodiscard]] const std::string &detail() const { return _detail; }
    [[nodiscard]] bool interactive() const { return _interactive; }
    [[nodiscard]] const std::string &prompt() const { return _prompt; }
    [[nodiscard]] bool prompt_secret() const { return _promptSecret; }
    [[nodiscard]] const std::string &log_path() const { return _logPath; }

    // Without its suffix, and empty for a run that is for no kernel in particular.
    [[nodiscard]] const std::string &version() const { return _version; }

    // A console redraws its whole document on every batch, so while nobody is
    // watching the output is held here and handed over in one piece.
    [[nodiscard]] bool watching() const { return _watching; }
    void set_watching(bool watching);

    void update();
    void download(const std::string &version);
    void extract(const std::string &version, const std::string &suffix);
    void patch(const std::string &version, const std::string &suffix);
    void revert(const std::string &version, const std::string &suffix);
    void configure(const std::string &version, const std::string &suffix);
    void build(const std::string &version, const std::string &suffix);

    void up_to_build(const std::string &version, const std::string &suffix);
    void everything(const std::string &version, const std::string &suffix);

    void install(const std::string &version, const std::string &suffix);
    void sign(const std::string &version, const std::string &suffix);
    void remove(const std::string &version, const std::string &suffix);
    void remove_installed(const std::string &version, const std::string &suffix);
    void remove_sources(const std::string &version, const std::string &suffix);
    void pull();

    [[nodiscard]] static BuildConfig::Choices choices(const std::string &version, const std::string &suffix);
    static void set_choices(const std::string &version, const std::string &suffix,
                            const BuildConfig::Choices &values);

    void provide_input(const std::string &input);
    void cancel();
    void dismiss();

    std::function<void()> changed;

    std::function<void(const std::string &)> output;

    std::function<void()> inputProvided;
    std::function<void(bool)> completed;

private:
    static std::string describe(int stages);

    static bool built(const std::string &version, const std::string &suffix);
    void start(const Options &options, const std::string &nothingToDo = {});
    void run(const std::shared_ptr<Workflow> &workflow);
    void append(const std::string &text);
    void flush();
    void measure();
    void finish(bool success);
    void release();
    void touch() const;

    [[nodiscard]] static Options options(const std::string &version, const std::string &suffix, int stages);

    ttk::Clock *_clock;
    ttk::Notifier *_notifier;
    DesktopAlert *_alert;
    std::shared_ptr<Workflow> _workflow;
    std::thread _thread;
    int _timer = 0;

    std::mutex _pendingMutex;
    std::string _pending;
    std::string _logPath;

    std::string _title;
    std::string _description;
    std::string _version;
    std::string _task;
    std::string _detail;
    std::string _prompt;
    bool _promptSecret = false;
    int _progress = -1;
    std::uint64_t _completed = 0;
    std::uint64_t _total = 0;
    std::uint64_t _sampled = 0;
    double _rate = 0;
    std::chrono::steady_clock::time_point _sampledAt;
    bool _running = false;
    bool _finished = false;
    bool _succeeded = false;
    bool _canceling = false;
    bool _interactive = false;
    bool _watching = true;
};


#endif //KERNELMGR_GUI_MODEL_WORKFLOWRUNNER_H
