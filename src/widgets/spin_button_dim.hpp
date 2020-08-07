#pragma once
#include <gtkmm.h>

namespace horizon {
class SpinButtonDim : public Gtk::SpinButton {
public:
    SpinButtonDim();

protected:
    int on_input(double *new_value) override;
    bool on_output() override;
};
} // namespace horizon
