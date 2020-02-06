#pragma once
#include "common/common.hpp"
#include "util/uuid.hpp"
#include <array>
#include <experimental/optional>
#include <gtkmm.h>
#include <set>
namespace horizon {

class FootprintGeneratorWindow : public Gtk::Window {
public:
    FootprintGeneratorWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x);
    static FootprintGeneratorWindow *create(Gtk::Window *p, class CorePackage *c);
    typedef sigc::signal<void> type_signal_generated;
    type_signal_generated signal_generated()
    {
        return s_signal_generated;
    }

private:
    Gtk::Stack *stack;
    CorePackage *core;
    Gtk::Button *generate_button;
    void update_can_generate();
    type_signal_generated s_signal_generated;
};
} // namespace horizon
