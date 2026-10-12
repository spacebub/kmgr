// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_DKMS_H
#define KERNELMGR_DKMS_H


#include <string>

#include "Command.h"
#include "Shell.h"

namespace Dkms {
inline bool available() {
    return Command::exists("dkms");
}

// Whether any module is built or installed for the kernel named.
inline bool present_for(const std::string &version) {
    return !Command("dkms status -k " + Shell::quote(version) + " 2>/dev/null").capture().empty();
}

// Takes every module back out of the kernel named and drops what was built for it. The
// trailing glob is meant to stay unquoted.
inline std::string removal(const std::string &version) {
    const std::string quoted = Shell::quote(version);

    return "dkms status -k " + quoted
           + " | while IFS= read -r entry; do"
             " module=$(printf '%s' \"$entry\" | cut -d',' -f1);"
             " [ -n \"$module\" ] && dkms remove \"$module\" -k "
           + quoted + "; done; rm -rfv /var/lib/dkms/*/kernel-" + quoted + "-*";
}
}


#endif //KERNELMGR_DKMS_H
