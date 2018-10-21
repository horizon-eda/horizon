#pragma once
#include <sigc++/sigc++.h>

namespace horizon {
class PreferencesProvider : public sigc::trackable {
public:
    PreferencesProvider();
    static PreferencesProvider &get();
    const class Preferences &get_prefs_ns() const;
    static const class Preferences &get_prefs();
    void set_prefs(Preferences &p);

    typedef sigc::signal<void> type_signal_changed;
    type_signal_changed signal_changed()
    {
        return s_signal_changed;
    }

private:
    class Preferences *prefs = nullptr;
    type_signal_changed s_signal_changed;
};

} // namespace horizon
