#pragma once
#include <array>
#include <gtkmm.h>
#include <set>
namespace horizon {

class ImpPreferencesWindow : public Gtk::Window {
    friend class CanvasPreferencesEditor;

public:
    ImpPreferencesWindow(class ImpPreferences *pr);

private:
    class ImpPreferences *preferences;
};
} // namespace horizon
