// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <filesystem>
#include "yyjson.h"

#include "BuildConfig.h"

namespace {
    constexpr auto FILENAME = ".kernelmgr-options";
    constexpr auto KEY_CONFIG = "config";
    constexpr auto KEY_PATCH = "patch";
    constexpr auto KEY_COMPILER = "compiler";
    constexpr auto KEY_FORCE = "force";

    std::string path_of(const Kernel::Version &version) {
        return Kernel::get_source_directory(version) + "/" + FILENAME;
    }

    std::string read_string(yyjson_val *root, const char *key) {
        yyjson_val *field = yyjson_obj_get(root, key);

        return field && yyjson_is_str(field) ? yyjson_get_str(field) : std::string{};
    }
}

std::optional<BuildConfig::Choices> BuildConfig::read(const Kernel::Version &version) {
    yyjson_doc *doc = yyjson_read_file(path_of(version).c_str(), YYJSON_READ_NOFLAG, nullptr, nullptr);

    // Missing and unreadable come to the same thing: nothing has been said.
    if (!doc) {
        return std::nullopt;
    }

    yyjson_val *root = yyjson_doc_get_root(doc);
    Choices choices;

    choices.config = read_string(root, KEY_CONFIG);
    choices.patch = read_string(root, KEY_PATCH);
    choices.compiler = toolchain_from(read_string(root, KEY_COMPILER));

    if (yyjson_val *force = yyjson_obj_get(root, KEY_FORCE); force && yyjson_is_bool(force)) {
        choices.force = yyjson_get_bool(force);
    }

    yyjson_doc_free(doc);

    return choices;
}

bool BuildConfig::write(const Kernel::Version &version, const Choices &choices) {
    if (!std::filesystem::is_directory(Kernel::get_source_directory(version))) {
        return false;
    }

    yyjson_mut_doc *doc = yyjson_mut_doc_new(nullptr);
    yyjson_mut_val *root = yyjson_mut_obj(doc);

    // Kept alive here: the writer holds the pointer until the file is written.
    const std::string compiler = toolchain_name(choices.compiler);

    yyjson_mut_doc_set_root(doc, root);

    yyjson_mut_obj_add_str(doc, root, KEY_CONFIG, choices.config.c_str());
    yyjson_mut_obj_add_str(doc, root, KEY_PATCH, choices.patch.c_str());
    yyjson_mut_obj_add_str(doc, root, KEY_COMPILER, compiler.c_str());
    yyjson_mut_obj_add_bool(doc, root, KEY_FORCE, choices.force);

    constexpr yyjson_write_flag flags = YYJSON_WRITE_PRETTY_TWO_SPACES;
    yyjson_write_err error;

    yyjson_mut_write_file(path_of(version).c_str(), doc, flags, nullptr, &error);
    yyjson_mut_doc_free(doc);

    return error.code == 0;
}
