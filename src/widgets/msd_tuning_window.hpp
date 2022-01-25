#pragma once
#include <gtkmm.h>
#include "util/changeable.hpp"
#include "util/msd.hpp"

namespace horizon {
class MSDTuningWindow : public Gtk::Window, public Changeable {
public:
    MSDTuningWindow();

    MSD::Params get_msd_params() const;

private:
    Gtk::SpinButton *sp_mass = nullptr;
    Gtk::SpinButton *sp_springyness = nullptr;
    Gtk::SpinButton *sp_damping = nullptr;
    Gtk::SpinButton *sp_time = nullptr;

    Gtk::DrawingArea *area = nullptr;

    Glib::RefPtr<Pango::Layout> layout;
    void create_layout();
    void reset();


    bool draw_graph(const Cairo::RefPtr<Cairo::Context> &cr);
};
} // namespace horizon
