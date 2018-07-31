#pragma once
#include <gtkmm.h>
#include "canvas/layer_display.hpp"

namespace horizon {
class CellRendererColorBox : public Gtk::CellRenderer {
public:
    CellRendererColorBox();
    virtual ~CellRendererColorBox(){};

    typedef Glib::Property<Gdk::RGBA> type_property_color;
    Glib::PropertyProxy<Gdk::RGBA> property_color()
    {
        return p_property_color.get_proxy();
    }

protected:
    virtual void render_vfunc(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget,
                              const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area,
                              Gtk::CellRendererState flags);

    virtual void get_preferred_width_vfunc(Gtk::Widget &widget, int &min_w, int &nat_w) const;

    virtual void get_preferred_height_vfunc(Gtk::Widget &widget, int &min_h, int &nat_h) const;

private:
    type_property_color p_property_color;
};
} // namespace horizon
