// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <fnmatch.h>
#include <ranges>
#include <filesystem>
#include <format>
#include <fstream>
#include <utility>
#include "PatchTask.h"
#include "core/Configuration.h"

namespace {
    std::vector<std::string> resolve(const std::filesystem::path &directory, const std::string &pattern) {
        std::vector<std::string> matches;
        const std::filesystem::path parent = directory / std::filesystem::path(pattern).parent_path();
        const std::string name = std::filesystem::path(pattern).filename();

        if (name.find('*') == std::string::npos && name.find('?') == std::string::npos) {
            if (std::filesystem::exists(parent / name)) {
                matches.push_back(parent / name);
            }

            return matches;
        }

        if (!std::filesystem::exists(parent)) {
            return matches;
        }

        for (const auto &entry : std::filesystem::directory_iterator(parent)) {
            if (entry.is_regular_file()
                && fnmatch(name.c_str(), entry.path().filename().string().c_str(), 0) == 0) {
                matches.push_back(entry.path());
            }
        }

        std::ranges::sort(matches);

        return matches;
    }
}

PatchTask::PatchTask(Kernel::Version version, std::string patch, const Direction direction)
        : ProcessTask("Patch task", 3), _version(std::move(version)), _patch(std::move(patch)),
          _direction(direction) {
}

std::string PatchTask::find_definition() const {
    const std::string base = Configuration::get()->baseDirectory;

    if (!_patch.empty()) {
        if (std::filesystem::exists(_patch)) {
            return _patch;
        }

        const std::filesystem::path supplied = std::filesystem::path(base) / _patch;

        return std::filesystem::exists(supplied) ? supplied.string() : std::string{};
    }

    return Kernel::find_patch(_version);
}

bool PatchTask::collect(const std::string &definition, std::vector<std::string> &patches) const {
    std::ifstream file(definition);

    if (!file.is_open()) {
        fail("Could not open " + definition);

        return false;
    }

    std::string directory;
    std::string line;

    while (std::getline(file, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) {
            line.pop_back();
        }

        if (line.empty() || line.starts_with('#')) {
            continue;
        }

        if (directory.empty()) {
            directory = line.starts_with('/') ? line : "/" + line;

            if (!std::filesystem::exists(directory)) {
                fail("Patch directory " + directory + " does not exist!");

                return false;
            }

            continue;
        }

        const std::vector<std::string> matches = resolve(directory, line);

        if (matches.empty()) {
            fail(std::string("No patch matching ").append(line).append(" in ").append(directory));

            return false;
        }

        patches.insert(patches.end(), matches.begin(), matches.end());
    }

    return true;
}

bool PatchTask::plan(const std::string &source) {
    if (Kernel::is_patched(_version)) {
        const std::string with = Kernel::patched_with(_version);

        report(with.empty()
            ? "Already patched, skipping."
            : "Already patched with " + with + ", skipping.");

        return true;
    }

    const std::string definition = find_definition();

    if (definition.empty()) {
        report("No patch definition found, skipping.");

        return true;
    }

    if (definition.ends_with(".sh")) {
        _steps.push_back(Step {
            .label = "Running patch script " + definition,
            .command = "sh " + definition,
            .directory = source,
            .elevated = false,
            .optional = false
        });

        _definition = definition;

        return true;
    }

    report("Using patch definition " + definition);

    std::vector<std::string> patches;

    if (!collect(definition, patches)) {
        return false;
    }

    if (patches.empty()) {
        report("Patch definition lists no patches, skipping.");

        return true;
    }

    for (const std::string &patch : patches) {
        _steps.push_back(Step {
            .label = "Applying " + std::filesystem::path(patch).filename().string(),
            .command = "patch -p1 -N --batch -i '" + patch + "'",
            .directory = source,
            .elevated = false,
            .optional = false
        });
    }

    _definition = definition;

    return true;
}

// The definition comes off the mark, since the file on disk may have changed since.
// Patches come out newest first, as a patch laid over another has to be lifted first.
bool PatchTask::plan_reverse(const std::string &source) {
    if (!Kernel::is_patched(_version)) {
        report("Not patched, nothing to take back.");

        return true;
    }

    const std::string definition = Kernel::patched_with(_version);

    if (definition.empty()) {
        fail("Nothing is recorded about how " + source + " was patched, so it cannot be taken back.");

        return false;
    }

    if (definition.ends_with(".sh")) {
        fail("Patched by the script " + definition + ", which only it knows how to undo.");

        return false;
    }

    report("Taking back " + definition);

    std::vector<std::string> patches;

    if (!collect(definition, patches)) {
        return false;
    }

    for (const std::string &patch : std::ranges::reverse_view(patches)) {
        _steps.push_back(Step {
            .label = "Taking back " + std::filesystem::path(patch).filename().string(),
            .command = "patch -p1 -R -N --batch -i '" + patch + "'",
            .directory = source,
            .elevated = false,
            .optional = false
        });
    }

    _definition = definition;

    return true;
}

bool PatchTask::prepare() {
    const std::string source = Kernel::get_source_directory(_version);

    if (!std::filesystem::exists(source)) {
        fail("Kernel directory " + source + " does not exist!");

        return false;
    }

    return _direction == Reverse ? plan_reverse(source) : plan(source);
}

// The mark changes only once every patch is in or out, so a run that stops part
// way through leaves it as it found it.
bool PatchTask::run() {
    if (!ProcessTask::run()) {
        return false;
    }

    if (_definition.empty()) {
        return true;
    }

    const bool marked = _direction == Reverse
        ? Kernel::clear_patched(_version)
        : Kernel::mark_patched(_version, _definition);

    if (!marked) {
        report("Could not " + std::string(_direction == Reverse ? "clear" : "write")
            + " the patch mark in " + Kernel::get_source_directory(_version) + ".");
    }

    return true;
}
