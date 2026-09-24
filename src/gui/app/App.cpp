// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#include "gui/app/App.h"
#include "gui/app/Look.h"
#include "ttk/dialogs/ConfirmDialog.h"
#include "ttk/dialogs/FilePickerDialog.h"
#include "ttk/draw/Theme.h"

namespace Embedded {
    extern const unsigned char mark48[];
    extern const std::size_t mark48Size;
}

namespace {
    BLImage decode(const unsigned char *bytes, const std::size_t size) {
        BLImage image;

        if (image.read_from_data(bytes, size) != BL_SUCCESS) {
            return {};
        }

        return image;
    }
}

App::App(ttk::Shell &shell)
    : _shell(shell),
      _catalog(&shell),
      _system(&shell),
      _workflow(&shell, &_notifier, &_alert),
      _settings(&_notifier),
      _logs(&_notifier),
      _picker(&_notifier),
      _reach{
          .window = shell,
          .notify = _notifier,
          .catalog = _catalog,
          .system = _system,
          .settings = _settings,
          .workflow = _workflow,
          .logs = _logs,
          // Wired by wire(), once the window can answer them.
          .touch = {},
          .go = {},
          .show = {},
          .cycleShade = {},
          .refresh = {},
          .ask = {},
          .pick = {},
      } {
    wire();
    build();
}

void App::wire() {
    _reach.touch = [this] { touch(); };
    _reach.go = [this](const Page page) { go(page); };
    _reach.show = [this](const std::string &version) { show(version); };
    _reach.cycleShade = [this] { cycle_shade(); };
    _reach.refresh = [this] {
        _catalog.refresh();
        _system.refresh();
    };
    _reach.ask = [this](const std::string &title, const std::string &body, const std::string &accept,
                        const bool danger, std::function<void()> accepted) {
        ask(title, body, accept, danger, std::move(accepted));
    };
    _reach.pick = [this](const std::string &title, const std::vector<std::string> &filters,
                         const bool directories, std::function<void(const std::string &)> chosen) {
        pick(title, filters, directories, std::move(chosen));
    };

    _catalog.changed = [this] { touch(); };
    _system.changed = [this] { touch(); };
    _settings.changed = [this] { touch(); };
    _logs.changed = [this] { touch(); };
    _picker.changed = [this] { touch(); };
    _notifier.changed = [this] { touch(); };

    _settings.saved = [this] {
        _system.refresh();
        _catalog.refresh();
    };

    _workflow.changed = [this] { touch(); };
    _workflow.output = [this](const std::string &text) { _sheet->write(text); };

    // Whether a password is held is only known by asking, and a wrong one is
    // dropped mid-run, so both an answer and a finished run redraw.
    _workflow.inputProvided = [this] { touch(); };
    _workflow.completed = [this](bool) {
        _catalog.refresh();
        _system.refresh();

        // Every run leaves a log behind, so anything showing their weight is stale.
        _logs.touch();
    };

    _shell.draggable = [this](const double x, const double y) {
        return !_dialogs->covered() && !_shell.ui().has_dismiss() && _bar->draggable(x, y);
    };
    _shell.closing = [this] { _shell.stop(); };
    _shell.shortcut = [this](const ttk::Key &pressed) { return shortcut(pressed); };
    _shell.shadeChanged = [this] {
        ttk::Shell::set_outline(ttk::Theme::palette().borderStrong);
        _shell.ui().damage_all();
        touch();
    };
    _shell.resized = [this](const double, const double) { _shell.ui().relayout(); };
}

void App::build() {
    ttk::Root &root = _shell.ui();

    auto bar = std::make_unique<TitleBar>(&_reach, decode(Embedded::mark48, Embedded::mark48Size));
    auto pages = std::make_unique<ttk::Widget>();

    _bar = bar.get();
    _pages = pages.get();

    Frame *frame = root.content()->append(std::make_unique<Frame>(_bar, _pages));

    frame->add(std::move(bar));
    frame->add(std::move(pages));

    _home = _pages->append(std::make_unique<HomePage>(&_reach));
    _kernels = _pages->append(std::make_unique<KernelsPage>(&_reach));
    _settingsPage = _pages->append(std::make_unique<SettingsPage>(&_reach));

    // Above the pages and below the dialogs.
    _sheet = root.content()->append(std::make_unique<WorkflowSheet>(&_reach));

    _dialogs = root.layer(ttk::Root::DIALOGS)->append(std::make_unique<ttk::DialogLayer>());
    _dialogs->closed = [this](const ttk::Dialog *gone) {
        if (gone == _pick) {
            _pick = nullptr;
        }
    };

    _buzzes = root.layer(ttk::Root::NOTICES)
                  ->append(std::make_unique<Buzzes>([this](const int id) { _notifier.dismiss(id); }));
    _tips = root.layer(ttk::Root::TIPS)->append(std::make_unique<ttk::Tips>());

    ttk::Shell::set_outline(ttk::Theme::palette().borderStrong);
}

void App::run() {
    ask_about_notifications();

    // Runs once per turn of the loop. With nothing in flight the loop blocks, so
    // an idle window costs nothing.
    _shell.settle = [this] {
        if (_dirty) {
            _dirty = false;
            sync();
        }

        const ttk::Widget *over = _shell.ui().hovered();
        const double x = _shell.ui().pointer_x();
        const double y = _shell.ui().pointer_y();

        if (over != nullptr && !over->hint.empty()) {
            _tips->point(over->hint, over->box(), x, y, ttk::Shell::now());
        } else {
            _tips->point({}, BLRect{}, x, y, ttk::Shell::now());
        }

        _buzzes->set_messages(_notifier.messages());
    };

    _shell.run();
}

void App::sync() {
    if (const bool wants = _picker.state().open; wants != (_pick != nullptr)) {
        if (wants) {
            _pick = _dialogs->show(std::make_unique<ttk::FilePickerDialog>(_picker));
        } else if (_dialogs->top() == _pick) {
            _dialogs->dismiss();
        } else {
            _pick = nullptr;
        }
    }

    _dialogs->sync();
    _bar->sync(_page, _system.update_available(), _system.release());

    _home->set_visible(_page == Page::Home);
    _kernels->set_visible(_page == Page::Kernels);
    _settingsPage->set_visible(_page == Page::Settings);

    _home->sync();
    _kernels->sync();
    _settingsPage->sync();
    _sheet->sync();
}

void App::touch() {
    _dirty = true;
}

void App::go(const Page page) {
    if (page == _page) {
        return;
    }

    _page = page;
    touch();
}

void App::show(const std::string &version) {
    _kernels->show(version);
    go(Page::Kernels);
}

void App::cycle_shade() {
    Look::cycle(&_notifier);
    ttk::Shell::set_outline(ttk::Theme::palette().borderStrong);
    _shell.ui().damage_all();
    touch();
}

void App::ask(const std::string &title, const std::string &body, const std::string &accept, const bool danger,
              std::function<void()> accepted) {
    _dialogs->show(std::make_unique<ttk::ConfirmDialog>(title, body, accept, danger,
                                                        [this, accepted = std::move(accepted)] {
        if (accepted) {
            accepted();
        }

        touch();
    }));
}

void App::pick(const std::string &title, const std::vector<std::string> &filters, const bool directories,
               std::function<void(const std::string &)> chosen) {
    const std::string remember = directories ? "directory" : "file";

    if (_picker.start_directory(remember).empty()) {
        _picker.remember_directory(remember, SystemStatus::base_directory());
    }

    _picker.open(title, filters, directories, directories, false, remember,
                 [this, chosen = std::move(chosen)](const std::vector<std::string> &paths, bool) {
        if (!paths.empty() && chosen) {
            chosen(paths.front());
        }

        touch();
    });
}

bool App::shortcut(const ttk::Key &pressed) {
    if (pressed.code == ttk::Code::Escape) {
        if (_dialogs->covered()) {
            _dialogs->close();

            return true;
        }

        if (_sheet->visible() && !_sheet->minimized()) {
            _sheet->escape();

            return true;
        }

        return false;
    }

    if (pressed.code == ttk::Code::Tab) {
        _shell.ui().focus_next(pressed.shift);

        return true;
    }

    return false;
}

// The answer is saved before the question is shown, so closing it unanswered
// counts as a no and it is never asked again.
void App::ask_about_notifications() {
    if (SettingsBridge::notifications_answered()) {
        return;
    }

    _settings.set_notifications(false);

    ask("Enable desktop notifications?",
        "Builds take a while. KernelManager can notify you when a run asks for your password and when it "
        "finishes, so you do not have to watch it. You can change this later in Settings.",
        "Enable", false, [this] { _settings.set_notifications(true); });
}
