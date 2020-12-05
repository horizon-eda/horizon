#pragma once
#include <gtkmm.h>
#include "parameter/set.hpp"
#include "util/changeable.hpp"

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

protected:
    virtual Gtk::Widget *create_extra_widget(ParameterID id);
    virtual void erase_cb(ParameterID id)
    {
    }

    Gtk::Widget *create_apply_all_button(ParameterID id);

private:
    Gtk::MenuButton *add_button = nullptr;
    Gtk::ListBox *listbox = nullptr;
    Gtk::Menu menu;
    std::map<ParameterID, Gtk::MenuItem &> menu_items;
    ParameterSet *parameter_set;
    Glib::RefPtr<Gtk::SizeGroup> sg_label;
    void update_menu();
    std::optional<std::string> apply_all_tooltip_text;

    type_signal_activate_last s_signal_activate_last;

protected:
    type_signal_apply_all s_signal_apply_all;
};
} // namespace horizon
