// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_STAGES_H
#define KERNELMGR_STAGES_H


#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#include "Parser.h"
#include "core/Configuration.h"
#include "core/Kernel.h"
#include "core/workflow/WorkflowFactory.h"

namespace Stages {
inline std::vector<std::string> cleans(const Arguments &arguments) {
    std::vector<std::string> wanted;

    if (!(arguments.flags & Parser::CLEAN)) {
        return wanted;
    }

    size_t start = 0;

    while (start <= arguments.clean.length()) {
        const size_t comma = arguments.clean.find(',', start);
        const size_t end = comma == std::string::npos ? arguments.clean.length() : comma;

        std::string word = arguments.clean.substr(start, end - start);

        std::erase_if(word, [](const unsigned char character) { return std::isspace(character); });

        if (!word.empty()) {
            wanted.push_back(word);
        }

        start = end + 1;
    }

    return wanted;
}

inline bool cleans(const Arguments &arguments, const std::string &what) {
    const std::vector<std::string> wanted = cleans(arguments);

    return std::ranges::find(wanted, what) != wanted.end();
}

inline std::string validate_clean(const Arguments &arguments) {
    for (const std::string &word : cleans(arguments)) {
        if (word != "all" && word != "archive" && word != "logs") {
            return word;
        }
    }

    return {};
}

inline bool has_config(const Arguments &arguments) {
    if (arguments.kernel.empty()) {
        return false;
    }

    try {
        Kernel::Version version = Kernel::get_version(arguments.kernel);
        version.suffix = arguments.suffix;

        return Kernel::is_configured(version);
    } catch (const std::exception &) {
        return false;
    }
}

inline int stages_from(const Arguments &arguments) {
    int stages = Options::NONE;

    const bool cleaning = (arguments.flags & (Parser::CLEAN | Parser::DELETE)) != 0;

    if (arguments.flags & Parser::PULL) {
        stages |= Options::PULL;
    }

    if (arguments.flags & Parser::REVERT) {
        return stages | Options::REVERT;
    }

    if (cleaning) {
        if (arguments.flags & Parser::CLEAN) {
            if (cleans(arguments, "all")) {
                stages |= Options::CLEAN_INSTALLED | Options::CLEAN_SOURCE;
            }

            if (cleans(arguments, "archive")) {
                stages |= Options::CLEAN_ARCHIVE;
            }

            // Logs are not a task, so asking for them alone queues nothing.
        }

        if (arguments.flags & Parser::DELETE) {
            stages |= Options::CLEAN_SOURCE;
        }

        return stages;
    }

    if (arguments.flags & Parser::DOWNLOAD) {
        stages |= Options::PREPARE;
    } else if (arguments.flags & Parser::RESUME) {
        stages |= Options::BUILD;

        // Resuming builds what is already in the directory, unless there is nothing to build with.
        if (!arguments.config.empty() || !has_config(arguments)) {
            stages |= Options::CONFIGURE;
        }
    } else if (!arguments.kernel.empty()) {
        stages |= Options::PREPARE | Options::MAKE;
    }

    if (arguments.flags & Parser::INSTALL || !arguments.oldkernel.empty()) {
        if (stages & (Options::BUILD | Options::MAKE)) {
            stages |= Options::INSTALL;
        }
    }

    if (arguments.flags & Parser::SIGN) {
        stages |= Options::SIGN;
    }

    if (arguments.flags & Parser::NOPATCH) {
        stages &= ~Options::PATCH;
    }

    const bool named = (arguments.flags & (Parser::CLANG | Parser::GCC | Parser::CUSTOM)) != 0;
    const Toolchain configured = Configuration::get()->compiler;

    if ((arguments.flags & Parser::CUSTOM) || (!named && configured == Toolchain::Custom)) {
        stages |= Options::CUSTOM;
    } else if ((arguments.flags & Parser::CLANG) || (!named && configured == Toolchain::Llvm)) {
        stages |= Options::CLANG;
    }

    if (arguments.flags & Parser::FORCE) {
        stages |= Options::FORCE;
    }

    // A replaced kernel is cleaned up once the new one is in place, never before.
    if (!arguments.oldkernel.empty() && (stages & Options::INSTALL) != 0) {
        stages |= Options::CLEAN_INSTALLED | Options::CLEAN_SOURCE;
    }

    return stages;
}
}

#endif //KERNELMGR_STAGES_H
