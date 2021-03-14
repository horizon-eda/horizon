#pragma once
#include <array>
#include <gtkmm.h>
#include <set>
namespace horizon {

class PreferencesWindow : public Gtk::Window {
public:
    PreferencesWindow(class Preferences &pr);
    void show_page(const std::string &pg);

private:
    class Preferences &preferences;
    Gtk::Stack *stack = nullptr;
};
} // namespace horizon
