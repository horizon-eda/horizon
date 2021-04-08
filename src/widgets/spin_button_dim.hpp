#pragma once
#include <gtkmm.h>

namespace horizon {
class SpinButtonDim : public Gtk::SpinButton {
public:
    SpinButtonDim();

protected:
    int on_input(double *new_value) override;
    bool on_output() override;
    void on_populate_popup(Gtk::Menu *menu) override;
};
} // namespace horizon
