// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_MODEL_SETTINGSBRIDGE_H
#define KERNELMGR_GUI_MODEL_SETTINGSBRIDGE_H


#include <functional>
#include <string>

#include "ttk/notices/Notifier.h"

class SettingsBridge {
public:
    explicit SettingsBridge(ttk::Notifier *notifier);

    std::string bootDirectory;
    std::string grubDirectory;
    std::string archiveFormat;
    std::string kernelCdn;
    std::string baseDirectory;
    std::string archiveDirectory;
    std::string elevationCommand;
    std::string customToolchainName;
    int jobs = 0;
    std::string compiler;

    std::string customFlags;

    bool notifications = false;

    [[nodiscard]] static int detected_jobs();

    // The AOCC path is a placeholder for the user to replace.
    [[nodiscard]] static std::string aocc_preset();

    void load();
    bool save();
    void forget_password();

    // Writes straight to the file: the startup question has no page open to press Save on.
    void set_notifications(bool enabled);

    [[nodiscard]] static bool password_remembered();
    [[nodiscard]] static bool notifications_answered();

    [[nodiscard]] int revision() const { return _revision; }

    std::function<void()> changed;
    std::function<void()> saved;

private:
    ttk::Notifier *_notifier;
    int _revision = 0;
};


#endif //KERNELMGR_GUI_MODEL_SETTINGSBRIDGE_H
