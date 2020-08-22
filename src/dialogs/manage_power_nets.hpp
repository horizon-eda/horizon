#pragma once
#include <gtkmm.h>
namespace horizon {


class ManagePowerNetsDialog : public Gtk::Dialog {
public:
    ManagePowerNetsDialog(Gtk::Window *parent, class Block &b);

private:
    Block &block;
    Gtk::ListBox *listbox = nullptr;
    void handle_add_power_net();
};
} // namespace horizon
