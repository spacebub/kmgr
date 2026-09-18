// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2024
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_EXTRACTTASK_H
#define KERNELMGR_EXTRACTTASK_H


#include "Task.h"
#include "core/KernelArchive.h"

class ExtractTask final : public Task {
public:
    explicit ExtractTask(KernelArchive *archive);
    explicit ExtractTask(const Kernel::Version &version);

    bool run() override;
    int position() override;
    void cancel() override;

private:
    std::unique_ptr<KernelArchive> _owned = nullptr;
    KernelArchive *_archive;
};


#endif //KERNELMGR_EXTRACTTASK_H
