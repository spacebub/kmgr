// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_MODEL_SYSTEMSTATUS_H
#define KERNELMGR_GUI_MODEL_SYSTEMSTATUS_H


#include <functional>
#include <string>
#include <thread>

#include "core/Kernel.h"
#include "ttk/util/Clock.h"

// The release feed can hang, so it is asked for off the interface thread.
class SystemStatus {
public:
    explicit SystemStatus(ttk::Clock *clock);
    ~SystemStatus();

    SystemStatus(const SystemStatus &) = delete;
    SystemStatus &operator=(const SystemStatus &) = delete;
    SystemStatus(SystemStatus &&) = delete;
    SystemStatus &operator=(SystemStatus &&) = delete;

    [[nodiscard]] std::string current() const;
    [[nodiscard]] std::string suffix() const;
    [[nodiscard]] std::string release() const;
    [[nodiscard]] const std::string &compiler() const { return _compiler; }
    [[nodiscard]] bool secure_boot() const { return _secureBoot; }
    [[nodiscard]] bool image_installed() const { return _imageInstalled; }
    [[nodiscard]] bool image_signed() const { return _imageSigned; }
    [[nodiscard]] std::string latest() const;
    [[nodiscard]] bool checking() const { return _checking; }
    [[nodiscard]] bool latest_known() const { return _known; }
    [[nodiscard]] bool update_available() const;
    [[nodiscard]] bool ahead() const;
    [[nodiscard]] static std::string base_directory();
    [[nodiscard]] static std::string archive_directory();

    void refresh();
    void check_latest();

    std::function<void()> changed;

private:
    ttk::Clock *_clock;

    Kernel::Version _current{};
    Kernel::Version _latest{};
    std::string _compiler;
    bool _secureBoot = false;
    bool _imageInstalled = false;
    bool _imageSigned = false;
    bool _checking = false;
    bool _known = false;

    std::thread _fetch;
};


#endif //KERNELMGR_GUI_MODEL_SYSTEMSTATUS_H
