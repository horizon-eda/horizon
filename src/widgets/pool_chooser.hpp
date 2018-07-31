#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"

namespace horizon {
class PoolChooserDialog : public Gtk::Dialog {
public:
    PoolChooserDialog(Gtk::Window *parent, const std::string &msg = "");
    void select_pool(const UUID &uu);
    UUID get_selected_pool();

private:
    Gtk::ListBox *lb = nullptr;
};
} // namespace horizon
