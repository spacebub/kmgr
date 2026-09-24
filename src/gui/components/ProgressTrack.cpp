// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include <algorithm>
#include <cmath>

#include "gui/components/Parts.h"
#include "gui/components/ProgressTrack.h"
#include "ttk/toolkit/Root.h"

namespace {
    constexpr double SWEEP_SECONDS = 1.15;
    constexpr double SWEEP_SHARE = 0.32;

    double in_out_quad(const double at) {
        return at < 0.5 ? 2.0 * at * at : 1.0 - (std::pow((-2.0 * at) + 2.0, 2.0) / 2.0);
    }
}

void ProgressTrack::set_value(const int value) {
    if (value == _value) {
        return;
    }

    const bool was = _value >= 0;

    _value = value;

    if (_value >= 0) {
        const auto share = static_cast<float>(std::clamp(_value, 0, 100) / 100.0);

        if (was && root() != nullptr) {
            _share.run(share, now(), 0.18, ttk::Anim::Curve::CubicOut);
        } else {
            _share.set(share);
        }
    }

    wake();
    invalidate();
}

void ProgressTrack::set_tone(const ttk::Theme::Tone tone) {
    if (tone.colour().value == _tone.colour().value) {
        return;
    }

    _tone = tone;
    invalidate();
}

void ProgressTrack::moved() {
    wake();
}

void ProgressTrack::paint(const ttk::Painter &painter) {
    const double radius = _box.h / 2.0;
    const BLRgba32 tone = _tone.colour();

    painter.round(_box, radius, ttk::Theme::palette().sunken);
    painter.push(_box);

    if (_value >= 0) {
        painter.round(BLRect{_box.x, _box.y, _box.w * _share.value(), _box.h}, radius, tone);
    } else {
        const double wide = _box.w * SWEEP_SHARE;
        const double x = _box.x - wide + ((_box.w + wide) * in_out_quad(_phase));

        painter.round(BLRect{x, _box.y, wide, _box.h}, radius, tone);
    }

    painter.pop();
}

bool ProgressTrack::advance(const double now) {
    _share.advance(now);

    if (_value < 0 && Parts::shown(this)) {
        if (_started == 0.0) {
            _started = now;
        }

        _phase = std::fmod((now - _started) / SWEEP_SECONDS, 1.0);
        invalidate();

        return true;
    }

    _started = 0.0;
    invalidate();

    return _share.live();
}
