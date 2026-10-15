// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Part of kernelmgr
 *
 * Copyright (c) 2026
 * Authors:
 *	spacebub <spacebubs@proton.me>
 */
#ifndef KERNELMGR_GUI_COMPONENTS_SETTINGROW_H
#define KERNELMGR_GUI_COMPONENTS_SETTINGROW_H


#include <functional>
#include <string>

#include "gui/components/PathField.h"
#include "ttk/toolkit/Widget.h"
#include "ttk/toolkit/controls/Label.h"

struct Reach;

class SettingRow : public ttk::Widget {
public:
    SettingRow(Reach *reach,
               const std::string &label,
               const std::string &description,
               std::function<void(const std::string &)> edited);

    SettingRow *browse(const std::string &title);
    SettingRow *placeholder(const std::string &text);
    SettingRow *mono(bool value);
    SettingRow *last(bool value = true);

    [[nodiscard]] const std::string &text() const { return _field->text(); }
    void set_text(const std::string &text) const { _field->set_text(text); }

    [[nodiscard]] double narrowest(ttk::Typeface &type) const;

    double natural_height(ttk::Typeface &type, double width) override;
    void arrange(ttk::Typeface &type) override;
    void paint(const ttk::Painter &painter) override;

private:
    static constexpr double CONTROL_WIDTH = 460.0;
    static constexpr double GAP = 24.0;
    static constexpr double BELOW = 10.0;

    [[nodiscard]] double texts_width(ttk::Typeface &type) const;
    [[nodiscard]] double texts_height(ttk::Typeface &type) const;
    [[nodiscard]] double column(ttk::Typeface &type) const;
    [[nodiscard]] bool stacked(ttk::Typeface &type, double width) const;
    void place_texts(ttk::Typeface &type, double y, double width);

    ttk::Label *_label = nullptr;
    ttk::Label *_description = nullptr;
    PathField *_field = nullptr;
    bool _last = false;
};


#endif //KERNELMGR_GUI_COMPONENTS_SETTINGROW_H
