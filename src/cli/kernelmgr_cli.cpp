// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Entry point for the console application
 *
 * Copyright (c) 2024
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */

#include <atomic>
#include <iostream>
#include <csignal>
#include <thread>

#include "Completion.h"
#include "Listing.h"
#include "Parser.h"
#include "Prompt.h"
#include "Reporter.h"
#include "Stages.h"
#include "core/Configuration.h"
#include "core/Log.h"
#include "core/workflow/WorkflowFactory.h"


static volatile std::sig_atomic_t s_abort;

static void signal_handler(int)
{
    s_abort = 1;
}

static int run(int argc, char *argv[]);
static int get_options(const Arguments &arguments, Options *options);
static int execute(const Arguments &arguments, const Options &options);

// A settings file that exists but cannot be read throws from any path below.
int main(const int argc, char *argv[]) {
    try {
        return run(argc, argv);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\n";

        return 1;
    }
}

int run(const int argc, char *argv[]) {
    Arguments arguments;

    if (!Parser::parse(argc, argv, &arguments)) {
        return 1;
    }

    if (arguments.flags & Parser::VERSION) {
        std::cout << "kernelmgr " << KERNELMGR_VERSION << "\n";

        return 0;
    }

    if (!arguments.completion.empty()) {
        return Completion::script(arguments.completion);
    }

    if (!arguments.completeFor.empty()) {
        if (!arguments.path.empty()) {
            Settings settings = *Configuration::get();
            settings.baseDirectory = arguments.path;
            Configuration::set(settings);
        }

        return Completion::candidates(arguments.completeFor);
    }

    // Listing only reads, so it runs before a base directory is settled on.
    if (arguments.flags & Parser::LIST) {
        if (!arguments.path.empty()) {
            Settings settings = *Configuration::get();
            settings.baseDirectory = arguments.path;
            Configuration::set(settings);
        }

        return Listing::run();
    }

    Options options;

    const int status = get_options(arguments, &options);

    if (status != 0) {
        return status;
    }

    return execute(arguments, options);
}

int get_options(const Arguments &arguments, Options *options) {
    try {
        if (!arguments.path.empty()) {
            std::error_code error;
            std::filesystem::create_directories(arguments.path, error);

            if (!std::filesystem::is_directory(arguments.path)) {
                std::cerr << "Could not use " << arguments.path << " as the base directory"
                    << (error ? ": " + error.message() : "") << "\n";

                return 1;
            }

            Settings settings = *Configuration::get();
            settings.baseDirectory = arguments.path;
            Configuration::set(settings);
        }

        if (const std::string unknown = Stages::validate_clean(arguments); !unknown.empty()) {
            std::cerr << "Cannot clean \"" << unknown << "\", expected all, archive or logs\n";

            return 1;
        }

        if (arguments.flags & Parser::AUTOUPDATE) {
            *options = WorkflowFactory::autoupdate();
            console().line("Updating to " + options->kernel
                + (options->suffix.empty() ? "" : "-" + options->suffix)
                + ", replacing " + options->oldKernel
                + (options->oldSuffix.empty() ? "" : "-" + options->oldSuffix));
        } else {
            options->kernel = arguments.kernel;
            options->suffix = arguments.suffix;
            options->oldKernel = arguments.oldkernel;
            options->oldSuffix = arguments.oldsuffix;
            options->config = arguments.config;
            options->patch = arguments.patch;
            options->stages = Stages::stages_from(arguments);
        }
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\n";

        return 1;
    }

    return 0;
}

int execute(const Arguments &arguments, const Options &options) {
    // Logs are not a stage, so they are cleared here and count as a request alone.
    const bool logs = Stages::cleans(arguments, "logs");

    if (options.stages == Options::NONE && !logs) {
        print_short_usage();

        return 1;
    }

    std::unique_ptr<Workflow> workflow;

    if (options.stages != Options::NONE) {
        try {
            workflow.reset(WorkflowFactory::create(options));
        } catch (const std::exception &ex) {
            std::cerr << ex.what() << "\n";

            return 1;
        }

        if (workflow->is_empty()) {
            workflow.reset();
        }
    }

    if (!workflow && !logs) {
        console().line("Nothing to do.");

        return 0;
    }

    if (!Prompt::confirm_clean(arguments, options)) {
        return 0;
    }

    // Cleared before the run rather than after it, so the run keeps its own log.
    if (logs) {
        const std::string kernel = Prompt::log_target(arguments);

        if (const std::uintmax_t removed = Log::clear(kernel); removed > 0) {
            console().line("Removed " + Reporter::human_size(static_cast<double>(removed))
                + " of logs" + (kernel.empty() ? "" : " for " + kernel) + ".");
        } else {
            console().line(kernel.empty()
                ? "There were no logs to remove."
                : "There were no logs for " + kernel + ".");
        }
    }

    if (!workflow) {
        return 0;
    }

    workflow->on_step_changed([](const std::string &label) { console().begin(label); });
    workflow->on_step_finished([](const bool success) { console().finish(success); });
    workflow->on_exception([](const std::string &message) {
        console().finish(false);
        console().line(message);
    });
    workflow->on_data_received([](const std::string &data) { console().output(data); });
    workflow->on_progress([](const ProgressArgs &args) { console().progress(args); });

    console().line(workflow->get_name());
    workflow->on_input_required([&workflow](const InputRequest &request) {
        workflow->provide_input(Prompt::read_line(request.secret));
    });

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::atomic<bool> running = true;
    std::thread poller([&workflow, &running] {
        while (running) {
            if (s_abort) {
                workflow->cancel();
            }

            workflow->poll();
            console().tick();

            std::this_thread::sleep_for(std::chrono::milliseconds(90));
        }
    });

    const bool success = workflow->run();

    running = false;
    poller.join();

    console().finish(success);
    console().line(success ? "Done." : "Failed.");

    if (const std::string log = workflow->get_log_path(); !log.empty()) {
        console().line("Log: " + log);
    }

    return success ? 0 : 1;
}
