// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_PATCHTASK_H
#define KERNELMGR_PATCHTASK_H


#include "ProcessTask.h"
#include "core/Kernel.h"

class PatchTask final : public ProcessTask {
public:
    enum Direction {
        Apply,
        Reverse
    };

private:
    Kernel::Version _version;
    std::string _patch;
    Direction _direction;

    std::string _definition;

    [[nodiscard]] std::string find_definition() const;
    bool collect(const std::string &definition, std::vector<std::string> &patches) const;
    bool plan(const std::string &source);
    bool plan_reverse(const std::string &source);

protected:
    bool prepare() override;

public:
    explicit PatchTask(Kernel::Version version, std::string patch = {}, Direction direction = Apply);

    bool run() override;
};


#endif //KERNELMGR_PATCHTASK_H
