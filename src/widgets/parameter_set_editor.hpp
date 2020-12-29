#pragma once
#include <gtkmm.h>
#include <set>
#include "parameter/set.hpp"
#include "util/changeable.hpp"
#include "util/list_accumulator.hpp"

namespace horizon {
class ParameterSetEditor : public Gtk::Box, public Changeable {
    friend class ParameterEditor;

public:
    ParameterSetEditor(ParameterSet *ps, bool populate_init = true);
    void populate();
    void focus_first();
    void set_button_margin_left(int margin);
    void add_or_set_parameter(ParameterID param, int64_t value);
    void set_has_apply_all(const std::string &tooltip_text);
    void set_has_apply_all_toggle(const std::string &tooltip_text);
    void set_apply_all(std::set<ParameterID> params);

    typedef sigc::signal<void> type_signal_activate_last;
    type_signal_activate_last signal_activate_last()
    {
        return s_signal_activate_last;
    }

    typedef sigc::signal<void, ParameterID> type_signal_apply_all;
    type_signal_apply_all signal_apply_all()
    {
        return s_signal_apply_all;
    }

    typedef sigc::signal<void, ParameterID, bool> type_signal_apply_all_toggled;
    type_signal_apply_all_toggled signal_apply_all_toggled()
    {
        return s_signal_apply_all_toggled;
    }

    typedef sigc::signal<Gtk::Widget *, ParameterID>::accumulated<list_accumulator<Gtk::Widget *, false>>
            type_signal_create_extra_widget;
    type_signal_create_extra_widget signal_create_extra_widget()
    {
        return s_signal_create_extra_widget;
    }

    type_signal_apply_all signal_remove_extra_widget()
    {
        return s_signal_remove_extra_widget;
    }

private:
    Gtk::Widget *create_apply_all_button(ParameterID id);
    Gtk::MenuButton *add_button = nullptr;
    Gtk::ListBox *listbox = nullptr;
    Gtk::Menu menu;
    std::map<ParameterID, Gtk::MenuItem &> menu_items;
    ParameterSet *parameter_set;
    Glib::RefPtr<Gtk::SizeGroup> sg_label;
    void update_menu();
    std::optional<std::string> apply_all_tooltip_text;
    bool apply_all_toggle = false;

    type_signal_activate_last s_signal_activate_last;

    type_signal_create_extra_widget s_signal_create_extra_widget;
    type_signal_apply_all s_signal_remove_extra_widget;

protected:
    type_signal_apply_all s_signal_apply_all;
    type_signal_apply_all_toggled s_signal_apply_all_toggled;
};
} // namespace horizon
