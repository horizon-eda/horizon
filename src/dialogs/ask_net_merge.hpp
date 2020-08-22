#pragma once
#include <gtkmm.h>
namespace horizon {


class AskNetMergeDialog : public Gtk::Dialog {
public:
    AskNetMergeDialog(Gtk::Window *parent, class Net &a, Net &b);

private:
    Net &net;
    Net &into;
};
} // namespace horizon
