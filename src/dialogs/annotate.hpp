#pragma once
#include <gtkmm.h>
namespace horizon {


class AnnotateDialog : public Gtk::Dialog {
public:
    AnnotateDialog(Gtk::Window *parent, class Schematic &s);

private:
    class Schematic &sch;
    Gtk::Switch *w_fill_gaps = nullptr;
    Gtk::Switch *w_keep = nullptr;
    Gtk::Switch *w_ignore_unknown = nullptr;
    Gtk::ComboBoxText *w_order = nullptr;
    Gtk::ComboBoxText *w_mode = nullptr;

    void ok_clicked();
};
} // namespace horizon
