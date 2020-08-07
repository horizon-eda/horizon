#pragma once
#include <gtkmm.h>

namespace horizon {

class SpinButtonAngle : public Gtk::SpinButton {
public:
    SpinButtonAngle();

protected:
    int on_input(double *new_value) override;
    bool on_output() override;
    void on_wrapped() override;
};
} // namespace horizon
