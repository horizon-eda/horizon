#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "util/uuid.hpp"
namespace horizon {


class ManageBusesDialog : public Gtk::Dialog {
public:
    ManageBusesDialog(Gtk::Window *parent, class Block &b);
    bool valid = false;


private:
    Block &block;
    Gtk::Stack *stack;
    Gtk::ToolButton *delete_button;
    void add_bus();
    void remove_bus();
    void update_bus_removable();


    void ok_clicked();
};
} // namespace horizon
