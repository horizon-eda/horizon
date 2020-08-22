#pragma once
#include "common/common.hpp"
#include "util/uuid.hpp"
#include <array>
#include <gtkmm.h>
#include <set>
namespace horizon {

class FootprintGeneratorWindow : public Gtk::Window {
public:
    FootprintGeneratorWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class CorePackage &c);
    static FootprintGeneratorWindow *create(Gtk::Window *p, class CorePackage &c);
    typedef sigc::signal<void> type_signal_generated;
    type_signal_generated signal_generated()
    {
        return s_signal_generated;
    }

private:
    CorePackage &core;
    Gtk::Stack *stack = nullptr;
    Gtk::Button *generate_button = nullptr;
    void update_can_generate();
    type_signal_generated s_signal_generated;
};
} // namespace horizon
