#pragma once
#include <array>
#include <gtkmm.h>
#include <set>
namespace horizon {

class PreferencesWindow : public Gtk::Window {
public:
    PreferencesWindow(class Preferences *pr);
    void open_pool(const std::string &path = "");

private:
    class Preferences *preferences;
    class PoolPreferencesEditor *pool_prefs_editor = nullptr;
    Gtk::Stack *stack = nullptr;
};
} // namespace horizon
