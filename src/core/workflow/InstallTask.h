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

protected:
    bool prepare() override;

public:
    explicit InstallTask(Kernel::Version version, Toolchain compiler = Toolchain::Gcc);
};


#endif //KERNELMGR_INSTALLTASK_H
