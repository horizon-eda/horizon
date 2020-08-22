#pragma once
#include <gtkmm.h>
namespace horizon {


class EditFrameDialog : public Gtk::Dialog {
public:
    EditFrameDialog(Gtk::Window *parent, class Frame &fr);

private:
    Frame &frame;
};
} // namespace horizon
