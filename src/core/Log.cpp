// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <mutex>
#include <vector>

#include "Log.h"
#include "Configuration.h"

namespace {
    std::mutex s_mutex;
    std::ofstream s_file;
    std::string s_path;
    Log::Sink s_sink;

    std::string decorate(const Log::Level level, const std::string &message) {
        switch (level) {
            case Log::Level::Step:
                return "\n==> " + message + "\n";
            case Log::Level::Output:
                return message;
            case Log::Level::Warning:
                return "[warning] " + message + "\n";
            case Log::Level::Error:
                return "[error] " + message + "\n";
            default:
                return message + "\n";
        }
    }
}

std::string Log::open(const std::string &kernel, const std::string &operation) {
    std::lock_guard lock(s_mutex);

    if (s_file.is_open()) {
        s_file.close();
    }

    std::filesystem::path directory = Log::directory();

    directory /= kernel.empty() ? "maintenance" : kernel;

    if (!operation.empty()) {
        directory /= operation;
    }

    std::error_code error;
    std::filesystem::create_directories(directory, error);

    if (error) {
        s_path.clear();

        return s_path;
    }

    s_path = std::format("{}/{:%Y%m%d-%H%M%S}.log",
                         directory.string(),
                         std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));

    s_file.open(s_path, std::ios::out | std::ios::trunc);

    return s_path;
}

void Log::close() {
    std::lock_guard lock(s_mutex);

    if (s_file.is_open()) {
        s_file.close();
    }
}

std::string Log::path() {
    std::lock_guard lock(s_mutex);

    return s_path;
}

namespace {
    std::vector<std::filesystem::path> targets(const std::string &kernel, const Log::Scope scope) {
        std::vector<std::filesystem::path> found;
        std::error_code error;

        for (std::filesystem::directory_iterator it(Log::directory(), error), end;
                !error && it != end; it.increment(error)) {
            if (!it->is_directory(error)) {
                continue;
            }

            const std::string name = it->path().filename().string();
            const bool covered = scope == Log::Scope::Build
                ? name == kernel
                : kernel.empty() || name == kernel || name.starts_with(kernel + "-");

            if (covered) {
                found.push_back(it->path());
            }
        }

        return found;
    }

    // Close any log a run is writing, so the file is not pulled out from under it.
    void release_under(const std::vector<std::filesystem::path> &going) {
        std::lock_guard lock(s_mutex);

        for (const std::filesystem::path &target : going) {
            if (s_file.is_open()
                    && (s_path == target.string() || s_path.starts_with(target.string() + "/"))) {
                s_file.close();
                s_path.clear();

                return;
            }
        }
    }

    int count_files(const std::filesystem::path &directory) {
        int files = 0;
        std::error_code error;

        for (std::filesystem::directory_iterator it(directory, error), end;
                !error && it != end; it.increment(error)) {
            if (it->is_regular_file(error)) {
                files++;
            }
        }

        return files;
    }

    int count_runs(const std::filesystem::path &directory) {
        int runs = 0;
        std::error_code error;

        for (std::filesystem::recursive_directory_iterator it(directory, error), end;
                !error && it != end; it.increment(error)) {
            if (it->is_regular_file(error)) {
                runs++;
            }
        }

        return runs;
    }

    std::uintmax_t weigh_directory(const std::filesystem::path &directory) {
        std::uintmax_t total = 0;
        std::error_code error;

        for (std::filesystem::recursive_directory_iterator it(directory, error), end;
                !error && it != end; it.increment(error)) {
            if (it->is_regular_file(error)) {
                total += it->file_size(error);
            }
        }

        return total;
    }
}

std::string Log::directory() {
    return Configuration::get()->baseDirectory + "/logs";
}

std::string Log::directory(const std::string &build) {
    return build.empty() ? directory() : directory() + "/" + build;
}

std::uintmax_t Log::weigh(const std::string &kernel, const Scope scope) {
    std::uintmax_t total = 0;

    for (const std::filesystem::path &target : targets(kernel, scope)) {
        total += weigh_directory(target);
    }

    return total;
}

std::uintmax_t Log::clear(const std::string &kernel, const Scope scope) {
    const std::vector<std::filesystem::path> found = targets(kernel, scope);

    release_under(found);

    std::uintmax_t removed = 0;

    for (const std::filesystem::path &target : found) {
        const std::uintmax_t weight = weigh_directory(target);
        std::error_code error;

        std::filesystem::remove_all(target, error);

        if (!error) {
            removed += weight;
        }
    }

    return removed;
}

std::vector<Log::Holding> Log::holdings() {
    std::vector<Holding> found;
    std::error_code error;

    for (std::filesystem::directory_iterator it(directory(), error), end;
            !error && it != end; it.increment(error)) {
        std::error_code inner;

        if (!it->is_directory(inner)) {
            continue;
        }

        found.push_back(Holding {
            .name = it->path().filename().string(),
            .size = weigh_directory(it->path()),
            .runs = count_runs(it->path())
        });
    }

    std::ranges::sort(found, [](const Holding &a, const Holding &b) {
        return a.size != b.size ? a.size > b.size : a.name < b.name;
    });

    return found;
}

// Runs of more than one step are filed under the build itself and come back unnamed.
std::vector<Log::Filed> Log::filed(const std::string &build) {
    const std::filesystem::path base = directory(build);
    std::vector<Filed> steps;
    Filed whole { .operation = {}, .size = 0, .runs = 0 };
    std::error_code error;

    for (std::filesystem::directory_iterator it(base, error), end; !error && it != end; it.increment(error)) {
        std::error_code inner;

        if (it->is_directory(inner)) {
            steps.push_back(Filed {
                .operation = it->path().filename().string(),
                .size = weigh_directory(it->path()),
                .runs = count_files(it->path())
            });
        } else if (it->is_regular_file(inner)) {
            whole.size += it->file_size(inner);
            whole.runs++;
        }
    }

    std::ranges::sort(steps, [](const Filed &a, const Filed &b) { return a.operation < b.operation; });

    if (whole.runs > 0) {
        steps.insert(steps.begin(), whole);
    }

    return steps;
}

std::uintmax_t Log::clear_step(const std::string &build, const std::string &operation) {
    const std::filesystem::path base = directory(build);

    if (!operation.empty()) {
        const std::filesystem::path target = base / operation;
        const std::uintmax_t weight = weigh_directory(target);
        std::error_code error;

        release_under({ target });
        std::filesystem::remove_all(target, error);

        return error ? 0 : weight;
    }

    // Collected first, since removing entries while the directory is walked is unsafe.
    std::vector<std::filesystem::path> files;
    std::error_code error;

    for (std::filesystem::directory_iterator it(base, error), end; !error && it != end; it.increment(error)) {
        std::error_code inner;

        if (it->is_regular_file(inner)) {
            files.push_back(it->path());
        }
    }

    release_under(files);

    std::uintmax_t removed = 0;

    for (const std::filesystem::path &file : files) {
        std::error_code failure;
        const std::uintmax_t weight = std::filesystem::file_size(file, failure);

        if (std::filesystem::remove(file, failure); !failure) {
            removed += weight;
        }
    }

    return removed;
}

void Log::write(const Level level, const std::string &message) {
    Sink sink;

    {
        std::lock_guard lock(s_mutex);

        if (s_file.is_open()) {
            s_file << decorate(level, message);
            s_file.flush();
        }

        sink = s_sink;
    }

    // Called outside the lock, since the sink may do anything.
    if (sink) {
        sink(level, message);
    }
}
