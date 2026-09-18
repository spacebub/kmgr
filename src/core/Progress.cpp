// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */

#include "Progress.h"


void Progress::set_total(const size_t total) {
    if (!_ended && _total > 0) {
        return;
    }

    _completed = 0;
    _total = total;
    _ended = false;
}

void Progress::on_progress(const std::function<void(const ProgressArgs &)> &callback) {
    _onProgress = callback;
}

void Progress::on_end(const std::function<void(const ProgressEndArgs &)> &callback) {
    _onEnd = callback;
}

void Progress::increment() {
    if (_ended) {
        return;
    }

    ++_completed;
}

void Progress::set_completed(const size_t completed) {
    if (_ended) {
        return;
    }

    _completed = completed;
}

void Progress::request_cancel() {
    _cancelRequested = true;
}

bool Progress::is_cancel_requested() const {
    return _cancelRequested;
}

void Progress::poll() const {
    if (_ended || !_onProgress) {
        return;
    }

    _onProgress(ProgressArgs { .total = _total, .completed = _completed });
}

void Progress::end(const bool canceled) {
    if (_ended) {
        return;
    }

    _ended = true;
    _total = 0;
    _completed = 0;

    if (_onEnd) {
        _onEnd(ProgressEndArgs { .canceled = canceled });
    }
}
