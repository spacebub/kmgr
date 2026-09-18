// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_SIGNTASK_H
#define KERNELMGR_SIGNTASK_H


#include "ProcessTask.h"
#include "core/Kernel.h"

class SignTask final : public ProcessTask {
    Kernel::Version _version;

protected:
    bool prepare() override;

public:
    explicit SignTask(Kernel::Version version);
};


#endif //KERNELMGR_SIGNTASK_H
