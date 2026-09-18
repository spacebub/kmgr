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

#include "gui/app/Desk.h"
#include "gui/app/Look.h"
#include "gui/app/Reach.h"
#include "gui/components/Parts.h"
#include "gui/overlays/WorkflowSheet.h"
#include "ttk/draw/Theme.h"
#include "ttk/draw/Typeface.h"
#include "ttk/toolkit/Root.h"

namespace {
    constexpr double STRIP_WIDTH = 340.0;
    constexpr double STRIP_MARGIN = 18.0;

    const ttk::Theme::Palette &palette() {
        return ttk::Theme::of();
    }

    double beat(const double now, const double started, const int index, const double pause,
                const double swell, const double rest) {
        const double cycle = (pause * 2.0) + (swell * 2.0);
        const double at = std::fmod(std::max(0.0, now - started), cycle);
        const double from = index * pause;

        if (at < from || at >= from + (swell * 2.0)) {
            return rest;
        }

        const double along = at - from;

        return along < swell ? rest + ((1.0 - rest) * (along / swell))
                             : 1.0 - ((1.0 - rest) * ((along - swell) / swell));
    }
}

WorkflowSheet::Console::Console() {
    _scroll = append(std::make_unique<ttk::Scroll>());
    _view = static_cast<ttk::TextView *>(_scroll->hold(std::make_unique<ttk::TextView>()));
    _view->face(ttk::Typeface::mono, ttk::Theme::fontSmall)->ink([](size_t) { return palette().muted; });
}

void WorkflowSheet::Console::write(const std::string &text) {
    const bool atEnd = _lines.empty() || !_scroll->scrollable()
        || _scroll->offset() >= _scroll->reach() - _scroll->box().h - 1.0;

    std::string run = _tail + text;
    size_t from = 0;

    if (!_tail.empty()) {
        _held -= _tail.size();
        _lines.pop_back();
    }

    while (true) {
        const size_t at = run.find('\n', from);

        if (at == std::string::npos) {
            break;
        }

        _lines.push_back(run.substr(from, at - from));
        _held += at - from;
        from = at + 1;
    }

    _tail = run.substr(from);

    if (!_tail.empty()) {
        _lines.push_back(_tail);
        _held += _tail.size();
    }

    while (_held > LIMIT && !_lines.empty()) {
        _held -= _lines.front().size();
        _lines.erase(_lines.begin());
    }

    _view->set_rows(_lines);

    if (root() != nullptr) {
        root()->relayout();
    }

    _follow = _follow || atEnd;
}

void WorkflowSheet::Console::clear() {
    _lines.clear();
    _tail.clear();
    _held = 0;
    _view->set_rows({});
    _scroll->scroll_to(0.0);
}

void WorkflowSheet::Console::arrange(ttk::Typeface &type) {
    _scroll->place(BLRect{_box.x + 12.0, _box.y + 12.0, std::max(0.0, _box.w - 24.0), std::max(0.0, _box.h - 24.0)},
                   type);

    if (_follow) {
        _follow = false;
        _scroll->scroll_to(_scroll->reach());
    }
}

void WorkflowSheet::Console::paint(const ttk::Painter &painter) {
    painter.round(_box, ttk::Theme::radiusSmall, palette().sunken);
    painter.outline(_box, ttk::Theme::radiusSmall, 1.0, palette().border);
    Widget::paint(painter);
}

void WorkflowSheet::Throbber::paint(const ttk::Painter &painter) {
    const BLRgba32 ink = ttk::Theme::restated(tone, toneDark);

    for (int index = 0; index < 3; ++index) {
        painter.circle(BLPoint{_box.x + 2.5 + (index * 8.0), _box.y + 2.5}, 2.5,
                       ttk::Theme::alpha(ink, beat(_now, _started, index, 0.18, 0.26, 0.22)));
    }
}

bool WorkflowSheet::Throbber::advance(const double now) {
    if (_started == 0.0) {
        _started = now;
    }

    _now = now;
    invalidate();

    return live && Parts::shown(this);
}

WorkflowSheet::Strip::Strip(WorkflowSheet *sheet)
        : _sheet(sheet), _edge(palette().accent), _edgeDark(ttk::Theme::dark()) {
    _takesPointer = true;
    cursor = ttk::Cursor::Pointer;

    content = append(ttk::Box::column());
    content->pad(12.0)->spacing(8.0);
}

void WorkflowSheet::Strip::sync(const BLRgba32 edge) {
    if (edge.value != _edge.value) {
        _edge = edge;
        _edgeDark = ttk::Theme::dark();
        invalidate();
    }
}

BLRect WorkflowSheet::Strip::drawn() const {
    return BLRect{_box.x - 7.0, _box.y - 7.0, _box.w + 14.0, _box.h + 14.0};
}

void WorkflowSheet::Strip::paint(const ttk::Painter &painter) {
    // Nothing else is raised off the page, so the shadow is drawn here:
    // two rings, the outer fainter.
    const BLRgba32 shadow = palette().shadow;

    painter.round(BLRect{_box.x - 7.0, _box.y - 7.0, _box.w + 14.0, _box.h + 14.0}, ttk::Theme::radius + 7.0,
                  ttk::Theme::alpha(shadow, 0.45));
    painter.round(BLRect{_box.x - 3.0, _box.y - 3.0, _box.w + 6.0, _box.h + 6.0}, ttk::Theme::radius + 3.0,
                  ttk::Theme::alpha(shadow, 0.75));

    painter.round(_box, ttk::Theme::radius, palette().raised);
    painter.outline(_box, ttk::Theme::radius, 1.0,
                    ttk::Theme::alpha(ttk::Theme::restated(_edge, _edgeDark), holds_pointer() ? 1.0 : 0.6));

    Widget::paint(painter);
}

void WorkflowSheet::Strip::release(const ttk::Pointer &at) {
    if (holds(at.x, at.y)) {
        _sheet->set_minimized(false);
    }
}

void WorkflowSheet::Strip::enter() {
    Widget::enter();
    invalidate();
}

void WorkflowSheet::Strip::leave() {
    Widget::leave();
    invalidate();
}

WorkflowSheet::WorkflowSheet(Reach *reach) : _reach(reach) {
    _takesPointer = true;

    _panel = append(std::make_unique<ttk::Panel>());

    ttk::Box *column = _panel->append(ttk::Box::column());
    column->pad(20.0)->spacing(14.0);

    ttk::Box *head = column->append(ttk::Box::row());
    head->spacing(12.0)->cross(ttk::Box::Place::Centre);

    ttk::Box *titles = head->append(ttk::Box::column());
    titles->spacing(3.0);
    titles->stretch = 1.0;

    _title = titles->append(Parts::text("", palette().headingWeight, ttk::Theme::fontTitle, palette().text));

    ttk::Box *state = titles->append(ttk::Box::row());
    state->spacing(10.0)->cross(ttk::Box::Place::Centre);
    _task = state->append(Parts::text("", 400, ttk::Theme::fontSmall, palette().muted));

    _detail = state->append(Parts::text("", 400, ttk::Theme::fontSmall, palette().faint));
    _detail->mono();
    _detail->stretch = 1.0;

    _running = head->append(Parts::pill("Running", palette().accent, palette().accentSoft));
    _finished = head->append(Parts::pill("Done", palette().success, palette().successSoft));

    head->append(Parts::glyph_button(ttk::Glyphs::Glyph::Minimize, ttk::Theme::controlSmall,
                                     "Put it away and let it run", palette().muted, palette().text,
                                     palette().hover, [this] { set_minimized(true); }));

    _track = column->append(std::make_unique<ProgressTrack>());

    _console = column->append(std::make_unique<Console>());
    _console->stretch = 1.0;

    _asking = column->append(std::make_unique<ttk::Panel>());
    _asking->inset = true;
    _asking->fixedHeight = 62.0;

    ttk::Box *asking = _asking->append(ttk::Box::row());
    asking->pad(9.0)->spacing(10.0)->cross(ttk::Box::Place::Centre);

    _prompt = asking->append(Parts::text("Waiting for input", 400, ttk::Theme::fontSmall, palette().faint));
    _prompt->mono();

    _answer = asking->append(std::make_unique<ttk::Field>("", [](const std::string &) {}));
    _answer->mono()->placeholder("Answer");
    _answer->stretch = 1.0;
    _answer->accepted = [this] { send(); };

    _send = asking->append(std::make_unique<ttk::Button>("Send", [this] { send(); }));
    _send->kind(ttk::Button::Kind::Primary)->compact();

    ttk::Box *foot = column->append(ttk::Box::row());
    foot->spacing(8.0)->cross(ttk::Box::Place::Centre);

    _logPath = foot->append(Parts::text("", 400, ttk::Theme::fontSmall, palette().faint));
    _logPath->mono();
    _logPath->stretch = 1.0;

    foot->append(std::make_unique<ttk::Button>("Copy log path", [this] {
        Desk::copy(_reach->workflow.log_path());
        _reach->notify.info("Log path copied to the clipboard.");
    }))->kind(ttk::Button::Kind::Ghost)->compact();

    _cancel = foot->append(std::make_unique<ttk::Button>("Cancel", [this] {
        _reach->ask("Cancel " + _reach->workflow.title() + "?",
                    "The step that is running is stopped where it is. Anything it already changed stays changed.",
                    "Cancel it", true, [this] { _reach->workflow.cancel(); });
    }));
    _cancel->kind(ttk::Button::Kind::Danger)->compact();

    _close = foot->append(std::make_unique<ttk::Button>("Close", [this] { _reach->workflow.dismiss(); }));
    _close->kind(ttk::Button::Kind::Primary)->compact();

    // --- the strip ---

    _strip = append(std::make_unique<Strip>(this));

    ttk::Box *stripHead = _strip->content->append(ttk::Box::row());
    stripHead->spacing(8.0)->cross(ttk::Box::Place::Centre);

    _throbber = stripHead->append(std::make_unique<Throbber>());

    _stripTitle = stripHead->append(Parts::text("", 600, ttk::Theme::fontSmall, palette().text));
    _stripTitle->stretch = 1.0;

    stripHead->append(Parts::glyph_button(ttk::Glyphs::Glyph::Restore, ttk::Theme::controlSmall, "Bring it back",
                                          palette().muted, palette().text, palette().hover,
                                          [this] { set_minimized(false); }));

    _stripClose = stripHead->append(Parts::glyph_button(ttk::Glyphs::Glyph::Close, ttk::Theme::controlSmall,
                                                        "Dismiss", palette().muted, BLRgba32(0xffffffff),
                                                        palette().danger, [this] { _reach->workflow.dismiss(); }));

    _stripTrack = _strip->content->append(std::make_unique<ProgressTrack>());

    ttk::Box *stripFoot = _strip->content->append(ttk::Box::row());
    stripFoot->spacing(8.0)->cross(ttk::Box::Place::Centre);
    _stripTask = stripFoot->append(Parts::text("", 400, ttk::Theme::fontSmall, palette().muted));
    _stripTask->stretch = 1.0;
    _stripDetail = stripFoot->append(Parts::text("", 400, ttk::Theme::fontSmall, palette().faint));
    _stripDetail->mono();

    set_visible(false);
}

void WorkflowSheet::set_minimized(const bool value) {
    if (_minimized == value) {
        return;
    }

    _minimized = value;
    _panel->set_visible(!value);
    _strip->set_visible(value);
    _track->wake();
    _stripTrack->wake();
    _throbber->wake();

    if (root() != nullptr) {
        root()->damage_all();
        root()->relayout();
    }

    _reach->touch();
}

void WorkflowSheet::escape() {
    if (_reach->workflow.finished()) {
        _reach->workflow.dismiss();
    } else {
        set_minimized(true);
    }
}

void WorkflowSheet::send() {
    _reach->workflow.provide_input(_answer->text());
    _answer->set_text("");
}

void WorkflowSheet::write(const std::string &text) {
    _console->write(text);
}

void WorkflowSheet::arrange(ttk::Typeface &type) {
    const BLRect area{_box.x, _box.y + Look::barHeight, _box.w, std::max(0.0, _box.h - Look::barHeight)};
    const double margin = std::min(28.0, area.w * 0.04);

    // The question takes at most its share of the row, and the answer the rest.
    const double asking = std::max(0.0, area.w - (margin * 2.0) - 40.0 - 18.0);

    _prompt->fixedWidth = std::min(_prompt->natural_width(type), asking * 0.4);

    _panel->place(BLRect{area.x + margin, area.y + margin, std::max(0.0, area.w - (margin * 2.0)),
                         std::max(0.0, area.h - (margin * 2.0))},
                  type);

    const double wide = std::min(STRIP_WIDTH, area.w - (STRIP_MARGIN * 2.0));
    const double tall = _strip->content->natural_height(type, wide);

    _strip->place(BLRect{area.x + area.w - STRIP_MARGIN - wide, area.y + area.h - STRIP_MARGIN - tall, wide, tall},
                  type);
}

BLRect WorkflowSheet::drawn() const {
    if (_minimized) {
        return _strip->drawn();
    }

    return BLRect{_box.x, _box.y + Look::barHeight, _box.w, std::max(0.0, _box.h - Look::barHeight)};
}

void WorkflowSheet::paint(const ttk::Painter &painter) {
    if (!_minimized) {
        painter.fill(drawn(), palette().scrim);
    }

    Widget::paint(painter);
}

ttk::Widget *WorkflowSheet::at(const double x, const double y) {
    if (!visible()) {
        return nullptr;
    }

    if (_minimized) {
        return _strip->at(x, y);
    }

    if (y < _box.y + Look::barHeight) {
        return nullptr;
    }

    if (Widget *found = Widget::at(x, y); found != nullptr) {
        return found;
    }

    // Unclaimed presses still stop here, so the page under the sheet never gets them.
    return this;
}

void WorkflowSheet::sync() {
    const WorkflowRunner &workflow = _reach->workflow;

    if (const bool shown = workflow.active(); shown != visible()) {
        set_visible(shown);
        _track->wake();
        _stripTrack->wake();

        if (root() != nullptr) {
            root()->damage_all();
        }
    }

    if (workflow.running() && !_wasRunning) {
        _console->clear();
        set_minimized(false);
    }

    _wasRunning = workflow.running();

    // Nothing that is waiting on an answer stays behind the strip.
    if (const bool prompting = !workflow.prompt().empty(); prompting && !_hadPrompt) {
        set_minimized(false);
    }

    _hadPrompt = !workflow.prompt().empty();

    // Put away, the console is not drawn, so the run holds its output until the sheet is back.
    _reach->workflow.set_watching(workflow.active() && !_minimized);

    if (!workflow.active()) {
        return;
    }

    const BLRgba32 verdict = workflow.finished() ? (workflow.succeeded() ? palette().success : palette().danger)
                           : workflow.canceling() ? palette().warning
                                                  : palette().accent;
    const BLRgba32 said = workflow.finished() ? verdict : palette().muted;

    _title->set_text(workflow.title());
    _task->set_text(workflow.task());
    _task->tone(said);
    _detail->set_visible(!workflow.detail().empty());
    _detail->set_text(workflow.detail());

    _running->set_visible(workflow.running());
    _running->set_text(workflow.canceling() ? "Canceling" : "Running");
    _running->tones(workflow.canceling() ? palette().warning : palette().accent,
                    workflow.canceling() ? palette().warningSoft : palette().accentSoft);
    _running->invalidate();

    _finished->set_visible(workflow.finished());
    _finished->set_text(workflow.succeeded() ? "Done" : "Failed");
    _finished->tones(workflow.succeeded() ? palette().success : palette().danger,
                     workflow.succeeded() ? palette().successSoft : palette().dangerSoft);
    _finished->invalidate();

    const int progress = workflow.running() ? workflow.progress() : 100;
    const BLRgba32 tone = workflow.finished() ? verdict : palette().accent;

    _track->set_value(progress);
    _track->set_tone(tone);

    _asking->set_visible(workflow.interactive() || !workflow.prompt().empty());
    _prompt->set_text(workflow.prompt().empty() ? "Waiting for input" : workflow.prompt());
    _prompt->tone(workflow.prompt().empty() ? palette().faint : palette().text);
    _answer->placeholder(workflow.prompt_secret() ? "Password" : "Answer");
    _answer->secret(workflow.prompt_secret());
    _send->set_enabled(workflow.running());

    _logPath->set_text(workflow.log_path());
    _cancel->set_visible(workflow.running());
    _cancel->set_enabled(!workflow.canceling());
    _close->set_visible(workflow.finished());

    _strip->sync(verdict);
    _throbber->set_visible(workflow.running());
    _throbber->tone = workflow.canceling() ? palette().warning : palette().accent;
    _throbber->toneDark = ttk::Theme::dark();

    if (const bool live = workflow.running() && _minimized; live != _throbber->live) {
        _throbber->live = live;
        _throbber->wake();
    }

    _track->wake();
    _stripTrack->wake();

    // A change in any of these moves what is beside it, so the sheet is laid out again.
    const std::string shape = workflow.title() + '\n' + workflow.prompt() + '\n' + workflow.log_path()
        + (workflow.running() ? "r" : "") + (workflow.finished() ? "f" : "") + (workflow.canceling() ? "c" : "")
        + (workflow.interactive() ? "i" : "") + (workflow.detail().empty() ? "" : "d");

    if (shape != _shape) {
        _shape = shape;

        if (root() != nullptr) {
            root()->relayout();
        }
    }

    _stripTitle->set_text(workflow.title());
    _stripClose->set_visible(workflow.finished());
    _stripTrack->set_value(progress);
    _stripTrack->set_tone(tone);
    _stripTask->set_text(workflow.canceling() ? "Canceling" : workflow.task());
    _stripTask->tone(said);
    _stripDetail->set_visible(!workflow.detail().empty());
    _stripDetail->set_text(workflow.detail());
}
