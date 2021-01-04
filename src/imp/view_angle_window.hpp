#pragma once
#include <gtkmm.h>

namespace horizon {
class ViewAngleWindow : public Gtk::Window {
public:
    ViewAngleWindow();
    class SpinButtonAngle &get_spinbutton();

private:
    class SpinButtonAngle *sp = nullptr;
};
} // namespace horizon
