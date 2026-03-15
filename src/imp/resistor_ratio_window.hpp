#pragma once
#include <gtkmm.h>
#include "util/window_state_store.hpp"

namespace horizon {


class ResistorRatioWindow : public Gtk::Window {
public:
    struct ResistorInfo {
        std::string refdes;
        double value;
        std::string value_formatted;
    };
    ResistorRatioWindow(const ResistorInfo &r1a, const ResistorInfo &r2a);


private:
    ResistorInfo r1;
    ResistorInfo r2;
    Gtk::Label *r1_label = nullptr;
    Gtk::Label *r2_label = nullptr;

    class FractionLabel;
    FractionLabel *vdiv_fraction_label = nullptr;

    Gtk::SpinButton *vdiv_lhs_sp = nullptr;
    Gtk::SpinButton *vdiv_rhs_sp = nullptr;

    FractionLabel *ratio_fraction_label = nullptr;

    Gtk::SpinButton *ratio_lhs_sp = nullptr;
    Gtk::SpinButton *ratio_rhs_sp = nullptr;

    void update();
    Gtk::SpinButton &make_sp();

    bool changing = false;
    void update_lhs();
    void update_rhs();

    WindowStateStore state_store;
};
} // namespace horizon
