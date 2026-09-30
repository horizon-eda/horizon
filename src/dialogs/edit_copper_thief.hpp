#pragma once
#include <gtkmm.h>

namespace horizon {
class CopperThief;

class EditCopperThiefDialog : public Gtk::Dialog {
public:
    EditCopperThiefDialog(Gtk::Window *parent, CopperThief &t, bool add_mode);
    bool delete_requested = false;

private:
    CopperThief &thief;
};
} // namespace horizon
