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

// Both throw on any failure, an HTTP error status included, with the URL in the message.
class Download {
    std::string _url;

public:
    explicit Download(std::string url);

    [[nodiscard]] std::string get_page() const;
    void perform(const std::string &destination, Progress *counter) const;
};


#endif //KERNELMGR_DOWNLOAD_H
