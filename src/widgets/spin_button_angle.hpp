#pragma once
#include <gtkmm.h>

namespace horizon {

class SpinButtonAngle : public Gtk::SpinButton {
public:
    SpinButtonAngle();

protected:
    virtual int on_input(double *new_value);
    virtual bool on_output();
    virtual void on_wrapped();
};
} // namespace horizon
