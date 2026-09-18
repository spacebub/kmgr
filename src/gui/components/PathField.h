// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_COMPONENTS_PATHFIELD_H
#define KERNELMGR_GUI_COMPONENTS_PATHFIELD_H


#include <functional>
#include <string>
#include <vector>

#include "ttk/toolkit/controls/Field.h"

struct Reach;

class PathField : public ttk::Field {
public:
    PathField(Reach *reach, const std::string &label, std::function<void(const std::string &)> edited);

    PathField *browse(std::string title, std::vector<std::string> filters, bool directories);

private:
    Reach *_reach;
    std::function<void(const std::string &)> _edited;
    std::string _title;
    std::vector<std::string> _filters;
    bool _directories = false;
};


#endif //KERNELMGR_GUI_COMPONENTS_PATHFIELD_H
