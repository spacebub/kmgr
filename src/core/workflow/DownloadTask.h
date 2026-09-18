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

class DownloadTask final : public Task {
public:
    DownloadTask(const std::string &url,
                 const std::string &destination);

    bool run() override;
    int position() override;
    void cancel() override;

private:
    std::string _url;
    std::string _destination;
};


#endif //KERNELMGR_DOWNLOADTASK_H
