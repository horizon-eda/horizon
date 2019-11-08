#pragma once
#include <gtkmm.h>
namespace horizon {

class SelectionFilterDialog : public Gtk::Window {
public:
    SelectionFilterDialog(Gtk::Window *parent, class SelectionFilter *sf, class Core *c);

private:
    SelectionFilter *selection_filter;
    Core *core;
    Gtk::ListBox *listbox = nullptr;
    std::vector<Gtk::CheckButton *> checkbuttons;
    Gtk::Button *reset_button = nullptr;
    void update_reset_button_sensitivity();
};
} // namespace horizon
