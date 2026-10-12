// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_INSTALLTASK_H
#define KERNELMGR_INSTALLTASK_H


#include "ProcessTask.h"
#include "core/Kernel.h"
#include "core/Toolchain.h"

class InstallTask final : public ProcessTask {
    Kernel::Version _version;
    Toolchain _compiler;
    // The kernel this one takes the place of, if any. What it had, the new one must have too.
    Kernel::Version _replaced;

protected:
    bool prepare() override;

public:
    explicit InstallTask(Kernel::Version version, Toolchain compiler = Toolchain::Gcc, Kernel::Version replaced = {});

    [[nodiscard]] bool is_critical() const override { return true; }
};


#endif //KERNELMGR_INSTALLTASK_H
