#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "parameter/set.hpp"

namespace horizon {

class EditViaDialog : public Gtk::Dialog {
public:
    EditViaDialog(Gtk::Window *parent, std::set<class Via *> &vias, class ViaPadstackProvider &vpp);
    bool valid = false;

private:
    class MyParameterSetEditor *editor = nullptr;
    Gtk::CheckButton *cb_from_rules = nullptr;
    class ViaPadstackButton *button_vp = nullptr;
    void update_sensitive();
};
} // namespace horizon
