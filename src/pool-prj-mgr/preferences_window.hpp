#pragma once
#include <array>
#include <gtkmm.h>
#include <set>
namespace horizon {

class PreferencesWindow : public Gtk::Window {
    friend class CanvasPreferencesEditor;

public:
    PreferencesWindow(class Preferences *pr);

private:
    class Preferences *preferences;
};
} // namespace horizon
