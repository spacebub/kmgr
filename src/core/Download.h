// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2024
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_DOWNLOAD_H
#define KERNELMGR_DOWNLOAD_H


#include <string>

#include "Progress.h"

class Download {
    std::string _url;

public:
    explicit Download(const std::string &url);
    ~Download();

    [[nodiscard]] std::string get_page() const;
    void perform(const std::string &destination,
                 Progress *counter) const;
};


#endif //KERNELMGR_DOWNLOAD_H
