#pragma once
#include "common/common.hpp"
#include "parameter/set.hpp"
#include "util/uuid.hpp"
#include <array>
#include <gtkmm.h>
#include <set>
namespace horizon {

class ParameterWindow : public Gtk::Window {
public:
    ParameterWindow(Gtk::Window *p, std::string *ppc, ParameterSet *ps);
    void set_can_apply(bool v);
    void set_error_message(const std::string &s);
    void add_button(Gtk::Widget *button);
    void insert_text(const std::string &text);

    typedef sigc::signal<void> type_signal_apply;
    type_signal_apply signal_apply()
    {
        return s_signal_apply;
    }

    class ParameterSetEditor *get_parameter_set_editor()
    {
        return parameter_set_editor;
    }

private:
    type_signal_apply s_signal_apply;
    Gtk::Button *apply_button = nullptr;
    Gtk::InfoBar *bar = nullptr;
    Gtk::Label *bar_label = nullptr;
    Gtk::Box *extra_button_box = nullptr;
    Gtk::TextView *tv = nullptr;

    class ParameterSetEditor *parameter_set_editor = nullptr;
};
} // namespace horizon
