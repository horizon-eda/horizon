#pragma once
#include <gtkmm.h>

namespace horizon {


class ManagePortsDialog : public Gtk::Dialog {
public:
    ManagePortsDialog(Gtk::Window *parent, class Block &b);
    bool valid = false;

private:
    Block &block;
    Gtk::ListBox *listbox = nullptr;
    void handle_add_port();

    void ok_clicked();
};
} // namespace horizon
