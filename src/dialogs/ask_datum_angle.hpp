#pragma once
#include <gtkmm.h>
#include <array>
#include <set>

namespace horizon {
class AskDatumAngleDialog : public Gtk::Dialog {
public:
    AskDatumAngleDialog(Gtk::Window *parent, const std::string &label);
    class SpinButtonAngle *sp = nullptr;

private:
};
} // namespace horizon
