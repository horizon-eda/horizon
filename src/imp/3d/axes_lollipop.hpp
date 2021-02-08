#pragma once
#include <gtkmm.h>

namespace horizon {
class AxesLollipop : public Gtk::DrawingArea {
public:
    AxesLollipop();
    void set_angles(float a, float b);

private:
    float alpha = 0;
    float beta = 0;
    Glib::RefPtr<Pango::Layout> layout;
    float size = 5;

    bool render(const Cairo::RefPtr<Cairo::Context> &cr);
    void create_layout();
};
} // namespace horizon
