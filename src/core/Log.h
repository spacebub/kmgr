// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_LOG_H
#define KERNELMGR_LOG_H


#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Log {
    enum class Level {
        Info,
        Step,
        Output,
        Warning,
        Error
    };

    using Sink = std::function<void (Level level, const std::string &message)>;

    std::string open(const std::string &kernel, const std::string &operation);
    void close();
    [[nodiscard]] std::string path();

    void write(Level level, const std::string &message);

    // A build without a suffix shares its version's name, so the scope must be said.
    enum class Scope {
        Family,   // A version and every build of it.
        Build     // One build directory only.
    };

    struct Filed {
        std::string operation;   // Empty for runs of more than one step.
        std::uintmax_t size;
        int runs;
    };

    // A run for no kernel in particular is filed under a name of its own.
    struct Holding {
        std::string name;
        std::uintmax_t size;
        int runs;
    };

    std::string directory();
    std::string directory(const std::string &build);

    std::uintmax_t weigh(const std::string &kernel = {}, Scope scope = Scope::Family);
    std::uintmax_t clear(const std::string &kernel = {}, Scope scope = Scope::Family);

    std::vector<Holding> holdings();
    std::vector<Filed> filed(const std::string &build);
    std::uintmax_t clear_step(const std::string &build, const std::string &operation);
}


#endif //KERNELMGR_LOG_H
