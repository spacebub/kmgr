// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "gui/app/Reach.h"
#include "gui/components/PathField.h"

PathField::PathField(Reach *reach, const std::string &label, std::function<void(const std::string &)> edited)
        : Field(label, edited), _reach(reach), _edited(std::move(edited)) {
    mono();
}

PathField *PathField::browse(std::string title, std::vector<std::string> filters, const bool directories) {
    _title = std::move(title);
    _filters = std::move(filters);
    _directories = directories;

    icon(ttk::Glyphs::Glyph::Folder, directories ? "Browse for a directory" : "Browse", [this] {
        _reach->pick(_title, _filters, _directories, [this](const std::string &path) {
            set_text(path);

            if (_edited) {
                _edited(path);
            }
        });
    });

    return this;
}
