// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_CLEANTASK_H
#define KERNELMGR_CLEANTASK_H


#include "ProcessTask.h"
#include "core/Kernel.h"

class CleanTask final : public ProcessTask {
public:
    enum Mode : unsigned {
        Installed = 1 << 0,
        Source = 1 << 1,
        Archive = 1 << 2,
        All = Installed | Source,
    };

private:
    Kernel::Version _version;
    unsigned _mode;

protected:
    bool prepare() override;

public:
    [[nodiscard]] bool is_critical() const override { return true; }

    CleanTask(Kernel::Version version, unsigned mode);
};


#endif //KERNELMGR_CLEANTASK_H
