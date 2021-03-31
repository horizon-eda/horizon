#pragma once
#include <gtkmm.h>

namespace horizon {

class StockInfoPreferencesEditor : public Gtk::Box {
public:
    StockInfoPreferencesEditor(class Preferences &prefs);

private:
    Preferences &preferences;
    Gtk::Stack *stack = nullptr;
    void update_editor();
};

} // namespace horizon
