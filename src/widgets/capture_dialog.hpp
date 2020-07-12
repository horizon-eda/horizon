#pragma once
#include <gtkmm.h>
#include "imp/action.hpp"

namespace horizon {
class CaptureDialog : public Gtk::Dialog {
public:
    CaptureDialog(Gtk::Window *parent);
    KeySequence keys;

private:
    Gtk::Label *capture_label = nullptr;
    void update();
};
} // namespace horizon
