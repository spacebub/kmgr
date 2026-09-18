// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "core/Configuration.h"
#include "core/SystemInfo.h"
#include "gui/model/SystemStatus.h"

SystemStatus::SystemStatus(ttk::Clock *clock) : _clock(clock) {
    refresh();
    check_latest();
}

SystemStatus::~SystemStatus() {
    if (_fetch.joinable()) {
        _fetch.join();
    }
}

void SystemStatus::refresh() {
    const Kernel current = Kernel::get_current();
    const auto [compiler, sbctl] = SystemInfo::get();

    _current = current.get_version();
    _compiler = compiler == Toolchain::Llvm ? "LLVM" : "GCC";
    _secureBoot = sbctl == Present;
    _imageInstalled = !current.get_image().empty();
    _imageSigned = current.is_signed();

    if (changed) {
        changed();
    }
}

// The answer is posted back only after the worker is joined, so a second check
// never races the first.
void SystemStatus::check_latest() {
    if (_checking) {
        return;
    }

    if (_fetch.joinable()) {
        _fetch.join();
    }

    _checking = true;

    if (changed) {
        changed();
    }

    _fetch = std::thread([this] {
        std::string version;

        try {
            version = Kernel::get_latest().get_version().get_string(Kernel::Version::INCLUDE_ZERO_REVISION);
        } catch (const std::exception &) {
        }

        _clock->post([this, version] {
            _checking = false;
            _known = !version.empty();

            if (_known) {
                _latest = Kernel::get_version(version);
            }

            if (changed) {
                changed();
            }
        });
    });
}

std::string SystemStatus::current() const {
    return _current.get_string(Kernel::Version::INCLUDE_ZERO_REVISION);
}

std::string SystemStatus::suffix() const {
    return _current.suffix;
}

std::string SystemStatus::release() const {
    return _current.get_string();
}

std::string SystemStatus::latest() const {
    return _known ? _latest.get_string(Kernel::Version::INCLUDE_ZERO_REVISION) : std::string{};
}

bool SystemStatus::update_available() const {
    return _known && _latest > _current;
}

bool SystemStatus::ahead() const {
    return _known && _latest < _current;
}

std::string SystemStatus::base_directory() {
    return Configuration::get()->baseDirectory;
}

std::string SystemStatus::archive_directory() {
    return Configuration::get()->archiveDirectory;
}
