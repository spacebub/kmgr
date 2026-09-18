// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2025
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_REPORTER_H
#define KERNELMGR_REPORTER_H


#include <algorithm>
#include <chrono>
#include <cmath>
#include <format>
#include <iostream>
#include <mutex>
#include <string>

#include "Ink.h"
#include "core/Progress.h"

class Reporter {
    static constexpr const char *SPINNER[] = {
        "\u280b", "\u2819", "\u2839", "\u2838", "\u283c",
        "\u2834", "\u2826", "\u2827", "\u2807", "\u280f"
    };

    std::mutex _mutex;
    std::string _label;
    std::string _detail;
    std::chrono::steady_clock::time_point _started;
    std::chrono::steady_clock::time_point _sampled;
    size_t _sampledBytes = 0;
    double _rate = 0;
    int _percentage = -1;
    int _frame = 0;
    bool _running = false;
    bool _expanded = false;    // The step has printed output of its own.
    bool _painted = false;     // A live line is on screen.
    bool _printed = false;     // Something has been left on screen for good.
    bool _interactive = Ink::interactive();

    static std::string paint(const std::string &text, const char *color) {
        return Ink::paint(text, color);
    }

public:
    static std::string human_size(const double bytes) {
        static constexpr const char *units[] = { "B", "KB", "MB", "GB" };
        double value = bytes;
        size_t unit = 0;

        while (value >= 1024.0 && unit + 1 < std::size(units)) {
            value /= 1024.0;
            ++unit;
        }

        return std::format("{:.1f} {}", value, units[unit]);
    }

private:
    static int width() {
        return Ink::columns();
    }

    static std::string elide(const std::string &text, const size_t room) {
        return Ink::elide(text, room);
    }

    [[nodiscard]] std::string elapsed() const {
        const double seconds = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - _started).count();

        return seconds >= 60
            ? std::format("{:.0f}m{:02.0f}s", std::floor(seconds / 60), std::fmod(seconds, 60))
            : std::format("{:.1f}s", seconds);
    }

    void repaint() {
        if (!_interactive || !_running || _expanded) {
            return;
        }

        const int columns = width();
        std::string trail;

        if (_percentage >= 0) {
            const int room = columns - static_cast<int>(_detail.length()) - 24;
            const int cells = std::clamp(room, 8, 24);
            const int filled = cells * _percentage / 100;

            std::string bar;

            for (int cell = 0; cell < cells; ++cell) {
                bar += cell < filled ? "\u2501" : "\u2500";
            }

            trail = "  " + paint(bar, CYAN)
                + std::format(" {:3}%", _percentage)
                + (_detail.empty() ? "" : "  " + paint(_detail, DIM));
        }

        // Measure the text, not the colour escapes.
        const size_t used = _percentage >= 0
            ? 3 + std::clamp(columns - static_cast<int>(_detail.length()) - 24, 8, 24)
                + 6 + (_detail.empty() ? 0 : _detail.length() + 2)
            : 3;

        std::cout << "\r" ERASE
            << " " << paint(SPINNER[_frame % std::size(SPINNER)], CYAN)
            << " " << elide(_label, columns > static_cast<int>(used) + 8 ? columns - used - 2 : 8)
            << trail << std::flush;
        _painted = true;
    }

    void clear() {
        if (_painted) {
            std::cout << "\r" ERASE;
            _painted = false;
        }
    }

    // The caller holds the lock.
    void close(const bool success) {
        if (!_running) {
            return;
        }

        clear();

        const std::string mark = success
            ? paint("\u2714", GREEN)
            : paint("\u2718", RED);

        std::cout << " " << mark << " " << elide(_label, width() - 14)
            << " " << paint(elapsed(), DIM) << "\n" << std::flush;

        _printed = true;
        _running = false;
        _expanded = false;
        _percentage = -1;
    }

public:
    void line(const std::string &text) {
        std::lock_guard lock(_mutex);

        clear();
        std::cout << text << "\n" << std::flush;
        _printed = true;
    }

    void begin(const std::string &label) {
        std::lock_guard lock(_mutex);

        close(true);

        _label = label;
        _detail.clear();
        _percentage = -1;
        _rate = 0;
        _sampledBytes = 0;
        _started = std::chrono::steady_clock::now();
        _sampled = _started;
        _running = true;
        _expanded = false;

        repaint();
    }

    void finish(const bool success) {
        std::lock_guard lock(_mutex);

        close(success);
    }

    void output(const std::string &data) {
        std::lock_guard lock(_mutex);

        if (_running && !_expanded) {
            clear();

            // A step that prints becomes a block, so it is set off from the one above.
            if (_printed) {
                std::cout << "\n";
            }

            std::cout << " " << paint("\u25b8", DIM) << " " << _label << "\n";
            _expanded = true;
        }

        std::cout << data << std::flush;
        _printed = true;
    }

    void progress(const ProgressArgs &args) {
        std::lock_guard lock(_mutex);

        if (args.total == 0 || !_running) {
            return;
        }

        const auto now = std::chrono::steady_clock::now();

        if (const double seconds = std::chrono::duration<double>(now - _sampled).count(); seconds >= 0.35) {
            const double sample = args.completed >= _sampledBytes
                ? static_cast<double>(args.completed - _sampledBytes) / seconds
                : 0;

            _rate = _rate > 0 ? _rate * 0.6 + sample * 0.4 : sample;
            _sampledBytes = args.completed;
            _sampled = now;
        }

        _percentage = args.percentage();
        _detail = human_size(static_cast<double>(args.completed))
            + " / " + human_size(static_cast<double>(args.total))
            + (_rate > 1 ? "  " + human_size(_rate) + "/s" : "");

        repaint();
    }

    void tick() {
        std::lock_guard lock(_mutex);

        _frame++;
        repaint();
    }

    [[nodiscard]] bool interactive() const {
        return _interactive;
    }
};


inline Reporter &console() {
    static Reporter instance;

    return instance;
}

#endif //KERNELMGR_REPORTER_H
