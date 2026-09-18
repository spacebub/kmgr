// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_COMPONENTS_LOGSTEPS_H
#define KERNELMGR_GUI_COMPONENTS_LOGSTEPS_H


#include <string>
#include <vector>

#include "core/Log.h"
#include "ttk/toolkit/layout/Box.h"

struct Reach;

class LogSteps : public ttk::Box {
public:
    explicit LogSteps(Reach *reach);

    void set_steps(const std::string &build, const std::vector<Log::Filed> &steps);
    void set_idle(bool idle);

    [[nodiscard]] int count() const { return _count; }

    [[nodiscard]] bool loaded() const { return _loaded; }

    // A run of more than one step has no step name.
    [[nodiscard]] static std::string step_name(const std::string &operation);

    [[nodiscard]] static std::string step_phrase(const std::string &operation);

private:
    Reach *_reach;
    std::string _build;
    std::vector<ttk::Widget *> _bins;
    int _count = 0;
    bool _idle = true;
    bool _loaded = false;
};


#endif //KERNELMGR_GUI_COMPONENTS_LOGSTEPS_H
