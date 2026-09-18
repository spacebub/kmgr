// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <chrono>
#include <utility>

#include "core/Configuration.h"
#include "core/Kernel.h"
#include "core/SystemInfo.h"
#include "gui/model/Format.h"
#include "gui/model/WorkflowRunner.h"

namespace {
    constexpr double FLUSH_SECONDS = 0.16;

    // Output kept while nobody is watching. It is cut back only once it grows well
    // past this, since each cut moves everything that is kept.
    constexpr std::string::size_type OUTPUT_LIMIT = 65000;
}

WorkflowRunner::WorkflowRunner(ttk::Clock *clock, ttk::Notifier *notifier, DesktopAlert *alert)
        : _clock(clock), _notifier(notifier), _alert(alert) {}

WorkflowRunner::~WorkflowRunner() {
    if (_running) {
        _workflow->cancel();
    }

    if (_thread.joinable()) {
        _thread.join();
    }

    release();
}

void WorkflowRunner::touch() const {
    if (changed) {
        changed();
    }
}

void WorkflowRunner::set_watching(const bool watching) {
    if (watching == _watching) {
        return;
    }

    _watching = watching;

    // Posted rather than flushed here: a sync sets this, and would read the
    // handover half way through.
    if (_watching) {
        _clock->post([this] { flush(); });
    }
}

bool WorkflowRunner::built(const std::string &version, const std::string &suffix) {
    return Kernel::is_built(Kernel::get_version(version, suffix));
}

// A stored answer wins. Otherwise the compiler the tree records does, and only
// a tree that records none falls back to the settings.
BuildConfig::Choices WorkflowRunner::choices(const std::string &version, const std::string &suffix) {
    const Kernel::Version target = Kernel::get_version(version, suffix);

    if (const std::optional<BuildConfig::Choices> stored = BuildConfig::read(target)) {
        return *stored;
    }

    BuildConfig::Choices choices;
    choices.compiler = Kernel::built_with(target);

    if (choices.compiler == Toolchain::Unknown) {
        choices.compiler = Configuration::get()->compiler;
    }

    return choices;
}

void WorkflowRunner::set_choices(const std::string &version, const std::string &suffix,
                                 const BuildConfig::Choices &values) {
    // Unknown is saved as GCC, which is what a build with no answer is made with.
    BuildConfig::Choices choices = values;

    if (choices.compiler == Toolchain::Unknown) {
        choices.compiler = Toolchain::Gcc;
    }

    BuildConfig::write(Kernel::get_version(version, suffix), choices);
}

Options WorkflowRunner::options(const std::string &version, const std::string &suffix, const int stages) {
    const BuildConfig::Choices choices = WorkflowRunner::choices(version, suffix);
    Options options;

    options.kernel = version;
    options.suffix = suffix;
    options.config = choices.config;
    options.patch = choices.patch;
    options.stages = stages;

    if (choices.compiler == Toolchain::Llvm) {
        options.stages |= Options::CLANG;
    } else if (choices.compiler == Toolchain::Custom) {
        options.stages |= Options::CUSTOM;
    }

    if (choices.force) {
        options.stages |= Options::FORCE;
    }

    return options;
}

void WorkflowRunner::update() {
    try {
        start(WorkflowFactory::autoupdate());
    } catch (const std::exception &ex) {
        _notifier->info(ex.what(), "Nothing to update");
    }
}

void WorkflowRunner::download(const std::string &version) {
    Options request = options(version, {}, Options::DOWNLOAD);
    request.suffix.clear();

    start(request, "That archive is already downloaded.");
}

void WorkflowRunner::extract(const std::string &version, const std::string &suffix) {
    start(options(version, suffix, Options::EXTRACT));
}

void WorkflowRunner::patch(const std::string &version, const std::string &suffix) {
    start(options(version, suffix, Options::PATCH));
}

void WorkflowRunner::revert(const std::string &version, const std::string &suffix) {
    start(options(version, suffix, Options::REVERT));
}

void WorkflowRunner::configure(const std::string &version, const std::string &suffix) {
    start(options(version, suffix, Options::CONFIGURE));
}

void WorkflowRunner::build(const std::string &version, const std::string &suffix) {
    start(options(version, suffix, Options::BUILD));
}

// Patching skips itself when there is no patch definition.
void WorkflowRunner::up_to_build(const std::string &version, const std::string &suffix) {
    start(options(version, suffix, Options::PATCH | Options::CONFIGURE | Options::BUILD));
}

// A directory that already holds an image picks up at the install, as
// --resume does. Rebuilding it is left to the Build button.
void WorkflowRunner::everything(const std::string &version, const std::string &suffix) {
    int stages = Options::INSTALL;

    if (!built(version, suffix)) {
        stages |= Options::PATCH | Options::CONFIGURE | Options::BUILD;
    }

    if (SystemInfo::get().sbctlStatus == Present) {
        stages |= Options::SIGN;
    }

    start(options(version, suffix, stages));
}

void WorkflowRunner::install(const std::string &version, const std::string &suffix) {
    start(options(version, suffix, Options::INSTALL));
}

void WorkflowRunner::sign(const std::string &version, const std::string &suffix) {
    start(options(version, suffix, Options::SIGN));
}

void WorkflowRunner::remove(const std::string &version, const std::string &suffix) {
    start(options(version, suffix, Options::CLEAN_INSTALLED | Options::CLEAN_SOURCE));
}

void WorkflowRunner::remove_installed(const std::string &version, const std::string &suffix) {
    start(options(version, suffix, Options::CLEAN_INSTALLED));
}

void WorkflowRunner::remove_sources(const std::string &version, const std::string &suffix) {
    start(options(version, suffix, Options::CLEAN_SOURCE), "There is no build directory to delete.");
}

void WorkflowRunner::pull() {
    Options request;
    request.stages = Options::PULL;

    start(request, "No repositories to update.");
}

std::string WorkflowRunner::describe(const int stages) {
    if (stages & Options::INSTALL) {
        return stages & Options::BUILD ? "Build and install" : "Install";
    }

    if (stages & Options::BUILD) {
        return "Build";
    }

    if (stages & Options::CONFIGURE) {
        return "Configure";
    }

    if (stages & Options::REVERT) {
        return "Revert patch";
    }

    if (stages & Options::PATCH) {
        return "Patch";
    }

    if (stages & Options::EXTRACT) {
        return "Extract";
    }

    if (stages & Options::DOWNLOAD) {
        return "Download";
    }

    if (stages & Options::SIGN) {
        return "Sign";
    }

    if (stages & (Options::CLEAN_INSTALLED | Options::CLEAN_SOURCE | Options::CLEAN_ARCHIVE)) {
        return "Removal";
    }

    if (stages & Options::PULL) {
        return "Sources update";
    }

    return "Workflow";
}

void WorkflowRunner::start(const Options &options, const std::string &nothingToDo) {
    if (_running) {
        _notifier->warning("Another workflow is still running.");

        return;
    }

    std::shared_ptr<Workflow> workflow;

    try {
        workflow.reset(WorkflowFactory::create(options));
    } catch (const std::exception &ex) {
        _notifier->error(ex.what());

        return;
    }

    _description = describe(options.stages);

    // Taken from the workflow, which resolves a short version to the one the
    // catalog lists. The suffix comes off because a run reaches the sources of
    // every build of its version.
    _version = workflow->get_kernel();

    if (const std::string tail = "-" + options.suffix;
        !options.suffix.empty() && _version.ends_with(tail)) {
        _version.resize(_version.size() - tail.size());
    }

    if (workflow->is_empty()) {
        _notifier->info(nothingToDo.empty() ? "Nothing to do." : nothingToDo);

        return;
    }

    run(workflow);
}

// Callbacks arrive on the worker thread, everything they touch is posted back.
void WorkflowRunner::run(const std::shared_ptr<Workflow> &workflow) {
    {
        std::lock_guard lock(_pendingMutex);
        _pending.clear();
    }

    // The last run's thread has ended, since it posted the finish this follows.
    if (_thread.joinable()) {
        _thread.join();
    }

    _workflow = workflow;
    _title = workflow->get_name();
    _task = "Starting";
    _prompt.clear();
    _promptSecret = false;
    _progress = -1;
    _detail.clear();
    _completed = 0;
    _total = 0;
    _sampled = 0;
    _rate = 0;
    _sampledAt = std::chrono::steady_clock::now();
    _running = true;
    _finished = false;
    _succeeded = false;
    _canceling = false;
    _interactive = false;

    workflow->on_data_received([this](const std::string &data) { append(data); });
    workflow->on_exception([this](const std::string &message) { append("\n[error] " + message + "\n"); });
    workflow->on_task_complete([this](const std::string &message) { append("-- " + message + "\n"); });
    workflow->on_step_changed([this](const std::string &label) { append("==> " + label + "\n"); });

    workflow->on_task_changed([this](const std::string &name) {
        _clock->post([this, name] {
            _task = name;
            _progress = -1;
            _detail.clear();
            _completed = 0;
            _total = 0;
            _sampled = 0;
            _rate = 0;
            _sampledAt = std::chrono::steady_clock::now();

            touch();
        });
    });

    workflow->on_input_required([this](const InputRequest &request) {
        _clock->post([this, request] {
            _prompt = request.prompt;
            _promptSecret = request.secret;

            // Nothing moves until this is answered, so the desktop is told too and the
            // alert stays up.
            _alert->post(_description + " · " + _title,
                         _prompt.empty() ? "Waiting for input." : _prompt, DesktopAlert::Urgency::Critical);

            touch();
        });
    });

    workflow->on_progress([this](const ProgressArgs &args) {
        const int percentage = args.total == 0 ? -1 : args.percentage();
        const std::uint64_t completed = args.completed;
        const std::uint64_t total = args.total;

        _clock->post([this, percentage, completed, total] {
            _completed = completed;
            _total = total;

            if (percentage == _progress) {
                return;
            }

            _progress = percentage;
            touch();
        });
    });

    _timer = _clock->every(FLUSH_SECONDS, [this] { flush(); });

    _thread = std::thread([this, workflow] {
        const bool success = workflow->run();

        _clock->post([this, success] { finish(success); });
    });

    touch();
}

void WorkflowRunner::append(const std::string &text) {
    std::lock_guard lock(_pendingMutex);

    _pending += text;
}

void WorkflowRunner::flush() {
    std::string pending;

    {
        std::lock_guard lock(_pendingMutex);

        if (_pending.size() > 2 * OUTPUT_LIMIT) {
            _pending.erase(0, _pending.size() - OUTPUT_LIMIT);
        }

        // With nobody reading, output stays here until somebody is.
        if (_watching) {
            pending.swap(_pending);
        }
    }

    if (_running) {
        _workflow->poll();

        if (const std::string path = _workflow->get_log_path(); path != _logPath) {
            _logPath = path;
            touch();
        }

        if (const bool interactive = _workflow->is_interactive(); interactive != _interactive) {
            _interactive = interactive;
            touch();
        }

        measure();
    }

    if (pending.empty()) {
        return;
    }

    if (output) {
        output(pending);
    }
}

// The rate is sampled a few times a second and smoothed, otherwise it jitters
// too much to read.
void WorkflowRunner::measure() {
    if (_total == 0) {
        if (!_detail.empty()) {
            _detail.clear();
            touch();
        }

        return;
    }

    const auto now = std::chrono::steady_clock::now();

    if (const double seconds = std::chrono::duration<double>(now - _sampledAt).count(); seconds >= 0.35) {
        const double rate = _completed >= _sampled
            ? static_cast<double>(_completed - _sampled) / seconds
            : 0;

        _rate = _rate > 0 ? _rate * 0.6 + rate * 0.4 : rate;
        _sampled = _completed;
        _sampledAt = now;
    }

    std::string detail = Format::size(_completed) + " of " + Format::size(_total);

    if (_rate > 1) {
        detail += "  ·  " + Format::size(static_cast<std::uintmax_t>(_rate)) + "/s";

        if (const double left = (static_cast<double>(_total) - static_cast<double>(_completed)) / _rate;
            left > 1 && left < 86400) {
            detail += "  ·  " + std::to_string(static_cast<int>(left)) + "s left";
        }
    }

    if (detail != _detail) {
        _detail = detail;
        touch();
    }
}

void WorkflowRunner::provide_input(const std::string &input) {
    if (!_running) {
        return;
    }

    _workflow->provide_input(input);
    _alert->withdraw();

    _prompt.clear();
    _promptSecret = false;

    touch();

    if (inputProvided) {
        inputProvided();
    }
}

void WorkflowRunner::cancel() {
    if (!_running || _canceling) {
        return;
    }

    _canceling = true;
    _task = "Canceling";

    touch();

    _workflow->cancel();
}

void WorkflowRunner::dismiss() {
    if (_running) {
        return;
    }

    _finished = false;
    _title.clear();
    _version.clear();
    _task.clear();

    release();

    touch();
}

void WorkflowRunner::finish(const bool success) {
    const bool canceled = _canceling;

    _running = false;
    _finished = true;
    _succeeded = success;
    _canceling = false;
    _interactive = false;
    _progress = 100;
    _task = success ? "Done" : "Failed";

    _clock->cancel(_timer);
    _timer = 0;
    flush();

    // No buzz, since the sheet already says it. The desktop is still told, as a
    // build outlasts the attention of whoever started it.
    const std::string what = _description + " · " + _title;

    if (canceled) {
        _alert->post(what, "Canceled.");
    } else if (success) {
        _alert->post(what, "Finished.");
    } else {
        _alert->post(what, "Failed. The log has the rest of it.", DesktopAlert::Urgency::Critical);
    }

    touch();

    if (completed) {
        completed(success);
    }
}

void WorkflowRunner::release() {
    if (!_workflow) {
        return;
    }

    _workflow->on_data_received(nullptr);
    _workflow->on_exception(nullptr);
    _workflow->on_task_complete(nullptr);
    _workflow->on_task_changed(nullptr);
    _workflow->on_input_required(nullptr);
    _workflow->on_progress(nullptr);

    _workflow.reset();
}
