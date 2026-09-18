// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_CONFIGURETASK_H
#define KERNELMGR_CONFIGURETASK_H


#include "ProcessTask.h"
#include "core/Kernel.h"
#include "core/Toolchain.h"

class ConfigureTask final : public ProcessTask {
    Kernel::Version _version;
    std::string _config;
    Toolchain _compiler;

    [[nodiscard]] std::string find_config() const;

protected:
    bool prepare() override;

public:
    explicit ConfigureTask(Kernel::Version version, std::string config = {},
                           Toolchain compiler = Toolchain::Gcc);
};


#endif //KERNELMGR_CONFIGURETASK_H
