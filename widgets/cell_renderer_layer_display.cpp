#include "cell_renderer_layer_display.hpp"

namespace horizon {
	CellRendererLayerDisplay::CellRendererLayerDisplay(): Glib::ObjectBase(typeid(CellRendererLayerDisplay)),
			Gtk::CellRenderer(),
			p_property_color(*this, "color", Gdk::RGBA()),
			p_property_display_mode(*this, "display-mode", LayerDisplay::Mode::FILL) {
		property_mode() = Gtk::CELL_RENDERER_MODE_ACTIVATABLE;
	}

	void CellRendererLayerDisplay::get_preferred_height_vfunc(Gtk::Widget& widget,
                                              int& min_h,
                                              int& nat_h) const
	{
		min_h = nat_h = 16;
	}

	void CellRendererLayerDisplay::get_preferred_width_vfunc(Gtk::Widget& widget,
												 int& min_w,
												 int& nat_w) const
	{
		min_w = nat_w = 16;
	}

	void CellRendererLayerDisplay::render_vfunc( const Cairo::RefPtr<Cairo::Context>& cr,
                                 Gtk::Widget& widget,
                                 const Gdk::Rectangle& background_area,
                                 const Gdk::Rectangle& cell_area,
                                 Gtk::CellRendererState flags )
	{
		cr->save();
		const auto c = p_property_color.get_value();

		const unsigned int d = 16;
		cr->translate(cell_area.get_x()+(cell_area.get_width()-d)/2, cell_area.get_y()+(cell_area.get_height()-d)/2);
		cr->rectangle(0, 0, 16, 16);
		cr->set_source_rgb(0,0,0);
		cr->fill_preserve();
		cr->set_source_rgb(c.get_red(), c.get_green(), c.get_blue());
		cr->set_line_width(2);
		LayerDisplay::Mode dm = p_property_display_mode.get_value();
		if(dm == LayerDisplay::Mode::FILL || dm == LayerDisplay::Mode::FILL_ONLY) {
			cr->fill_preserve();
		}

		cr->save();
		if(dm == LayerDisplay::Mode::FILL_ONLY) {
			cr->set_source_rgb(0,0,0);
		}
		cr->stroke();
		cr->restore();

		if(dm == LayerDisplay::Mode::HATCH) {
			cr->move_to(0,16);
			cr->line_to(16,0);
			cr->stroke();
			cr->move_to(0,9);
			cr->line_to(9,0);
			cr->stroke();
			cr->move_to(7,16);
			cr->line_to(16,7);
			cr->stroke();
		}

		cr->restore();

	}

	bool CellRendererLayerDisplay::activate_vfunc(GdkEvent *event,
										Gtk::Widget &widget,
										const Glib::ustring &path,
										const Gdk::Rectangle &background_area,
										const Gdk::Rectangle &cell_area,
										Gtk::CellRendererState flags)
	{
		s_signal_activate.emit(path);
		return false;
	}

}
