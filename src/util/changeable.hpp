#pragma once
#include <sigc++/sigc++.h>

namespace horizon {
class Changeable {
public:
    typedef sigc::signal<void> type_signal_changed;
    type_signal_changed signal_changed()
    {
        return s_signal_changed;
    }

protected:
    type_signal_changed s_signal_changed;
};
} // namespace horizon
