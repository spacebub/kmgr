// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2024
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_DOWNLOADTASK_H
#define KERNELMGR_DOWNLOADTASK_H


#include "Task.h"
#include "core/KernelArchive.h"

// Fetched under a working name and only given its own once its digest matches the list
// kernel.org publishes beside it, so a half-written or altered file is never mistaken for
// the archive.
class DownloadTask final : public Task {
    KernelArchive _archive;

    bool verify(const std::string &file);

public:
    explicit DownloadTask(KernelArchive archive);

    bool run() override;
    int position() override;
    void cancel() override;
};


#endif //KERNELMGR_DOWNLOADTASK_H
