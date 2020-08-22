#pragma once
#include <gtkmm.h>

namespace horizon {
class EditKeepoutDialog : public Gtk::Dialog {
public:
    EditKeepoutDialog(Gtk::Window *parent, class Keepout &k, class IDocument &c, bool add_mode);

private:
    class Keepout &keepout;
};
} // namespace horizon
