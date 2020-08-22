#pragma once
#include <gtkmm.h>

namespace horizon {
class EditPlaneDialog : public Gtk::Dialog {
public:
    EditPlaneDialog(Gtk::Window *parent, class Plane &p, class Board &brd);

private:
    class Plane &plane;
};
} // namespace horizon
