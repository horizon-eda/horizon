#pragma once
#include <gtkmm.h>
namespace horizon {

class ManageNetClassesDialog : public Gtk::Dialog {
public:
    ManageNetClassesDialog(Gtk::Window *parent, class IBlockProvider &b);

private:
    IBlockProvider &blocks;
    Gtk::ListBox *listbox = nullptr;
    void handle_add_net_class();
};
} // namespace horizon
