// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_PROMPT_H
#define KERNELMGR_PROMPT_H


#include <string>
#include <cctype>
#include <cstdint>
#include <vector>
#include <termios.h>
#include <unistd.h>

#include "core/Kernel.h"
#include "core/KernelArchive.h"

#include "Parser.h"
#include "Reporter.h"
#include "Stages.h"
#include "core/Log.h"
#include "core/workflow/WorkflowFactory.h"

namespace Prompt {
inline std::string read_line(const bool secret) {
    termios original{};
    const bool hidden = secret && tcgetattr(STDIN_FILENO, &original) == 0;

    if (hidden) {
        termios hiddenAttributes = original;
        hiddenAttributes.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &hiddenAttributes);
    }

    std::string line;
    std::getline(std::cin, line);

    if (hidden) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
        std::cout << "\n";
    }

    return line;
}

inline bool confirmed(const std::string &question) {
    console().line(question);
    std::cout << "[y]es, [N]o: " << std::flush;

    std::string answer = read_line(false);

    for (char &character : answer) {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }

    return answer == "y" || answer == "yes";
}

// Resolved the way WorkflowFactory resolves it.
inline std::string clean_target(const Options &options) {
    const bool replaced = !options.oldKernel.empty();
    const std::string version = replaced ? options.oldKernel : options.kernel;
    const std::string suffix = replaced && !options.oldSuffix.empty()
        ? options.oldSuffix
        : options.suffix;

    Kernel::Version parsed = Kernel::get_version(version);
    parsed.suffix = suffix;

    return parsed.get_string();
}

inline std::string log_target(const Arguments &arguments) {
    if (arguments.kernel.empty()) {
        return {};
    }

    Kernel::Version version = Kernel::get_version(arguments.kernel);
    version.suffix = arguments.suffix;

    return version.get_string();
}

// Only a clean asked for on its own is confirmed. Cleaning up after an
// install or an autoupdate is part of that request.
inline bool confirm_clean(const Arguments &arguments, const Options &options) {
    if (!(arguments.flags & Parser::CLEAN)) {
        return true;
    }

    std::vector<std::string> going;

    if (options.stages & (Options::CLEAN_INSTALLED | Options::CLEAN_SOURCE)) {
        if (!options.kernel.empty() || !options.oldKernel.empty()) {
            going.push_back(clean_target(options) + " and everything installed with it");
        }
    }

    if (options.stages & Options::CLEAN_ARCHIVE) {
        going.emplace_back(options.kernel.empty()
            ? "every kernel archive"
            : KernelArchive(Kernel::get_version(options.kernel)).get_location());
    }

    if (Stages::cleans(arguments, "logs")) {
        const std::string kernel = log_target(arguments);

        if (const std::uintmax_t weight = Log::weigh(kernel); weight > 0) {
            going.push_back(Reporter::human_size(static_cast<double>(weight)) + " of logs"
                + (kernel.empty() ? " for every kernel" : " for " + kernel));
        }
    }

    if (going.empty()) {
        return true;
    }

    std::string what = going.front();

    for (size_t next = 1; next < going.size(); ++next) {
        what += next + 1 == going.size() ? " and " : ", ";
        what += going[next];
    }

    return confirmed("You are about to remove " + what + ", are you sure?");
}
}

#endif //KERNELMGR_PROMPT_H
