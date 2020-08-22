#pragma once
#include <gtkmm.h>
namespace horizon {

class ManageNetClassesDialog : public Gtk::Dialog {
public:
    ManageNetClassesDialog(Gtk::Window *parent, class Block &b);

private:
    Block &block;
    Gtk::ListBox *listbox = nullptr;
    void handle_add_net_class();
};
} // namespace horizon
