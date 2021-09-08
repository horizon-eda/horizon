#pragma once
#include <gtkmm.h>
namespace horizon {


class ManagePowerNetsDialog : public Gtk::Dialog {
public:
    ManagePowerNetsDialog(Gtk::Window *parent, class IBlockProvider &b);

private:
    IBlockProvider &blocks;
    Gtk::ListBox *listbox = nullptr;
    void handle_add_power_net();
};
} // namespace horizon
