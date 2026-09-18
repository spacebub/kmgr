// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef PROGRESS_H
#define PROGRESS_H


#include <atomic>
#include <functional>

struct ProgressEndArgs {
    bool canceled;
};

struct ProgressArgs {
    size_t total;
    size_t completed;

    [[nodiscard]] int percentage() const {
        if (total == 0) {
            return 0;
        }

        return static_cast<int>(completed * 100 / total);
    }
};

class Progress {
    size_t _total = 0;
    std::atomic<size_t> _completed = 0;
    std::atomic<bool> _cancelRequested = false;
    bool _ended = true;

    std::function<void (const ProgressArgs &args)> _onProgress = nullptr;
    std::function<void (const ProgressEndArgs &args)> _onEnd = nullptr;

public:
    void set_total(size_t total);
    void on_progress(const std::function<void (const ProgressArgs &args)> &callback);
    void on_end(const std::function<void (const ProgressEndArgs &args)> &callback);

    void increment();
    void set_completed(size_t completed);
    void end(bool canceled);
    void poll() const;

    void request_cancel();
    [[nodiscard]] bool is_cancel_requested() const;
};


#endif //PROGRESS_H
