#include <widgets/cell_renderer_color_box.hpp>

namespace horizon {
CellRendererColorBox::CellRendererColorBox()
    : Glib::ObjectBase(typeid(CellRendererColorBox)), Gtk::CellRenderer(), p_property_color(*this, "color", Gdk::RGBA())
{
}

void CellRendererColorBox::get_preferred_height_vfunc(Gtk::Widget &widget, int &min_h, int &nat_h) const
{
    min_h = nat_h = 16;
}

void CellRendererColorBox::get_preferred_width_vfunc(Gtk::Widget &widget, int &min_w, int &nat_w) const
{
    min_w = nat_w = 16;
}

void CellRendererColorBox::render_vfunc(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget,
                                        const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area,
                                        Gtk::CellRendererState flags)
{
    cr->save();
    const auto c = p_property_color.get_value();

    const unsigned int d = 16;
    cr->translate(cell_area.get_x() + (cell_area.get_width() - d) / 2,
                  cell_area.get_y() + (cell_area.get_height() - d) / 2);
    cr->rectangle(0, 0, 16, 16);
    cr->set_source_rgb(c.get_red(), c.get_green(), c.get_blue());
    cr->set_line_width(2);
    // cr->stroke_preserve();
    cr->fill();
    cr->restore();
}

} // namespace horizon
