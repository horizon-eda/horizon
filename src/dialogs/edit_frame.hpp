#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
namespace horizon {


class EditFrameDialog : public Gtk::Dialog {
public:
    EditFrameDialog(Gtk::Window *parent, class Frame *fr);
    bool valid = false;


private:
    Frame *frame = nullptr;

    void ok_clicked();
};
} // namespace horizon
