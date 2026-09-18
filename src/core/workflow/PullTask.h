// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef PULLTASK_H
#define PULLTASK_H


#include "ProcessTask.h"

class PullTask final : public ProcessTask {

protected:
    bool prepare() override;

public:
    explicit PullTask();
};


#endif //PULLTASK_H
