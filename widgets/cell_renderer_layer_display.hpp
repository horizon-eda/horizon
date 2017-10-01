#pragma once
#include <gtkmm.h>
#include "canvas/layer_display.hpp"

namespace horizon {
	class CellRendererLayerDisplay: public Gtk::CellRenderer {
		public:
			CellRendererLayerDisplay();
			virtual ~CellRendererLayerDisplay() {};

			typedef Glib::Property<Gdk::RGBA> type_property_color;
			Glib::PropertyProxy<Gdk::RGBA> property_color() { return p_property_color.get_proxy(); }
			typedef Glib::Property<LayerDisplay::Mode> type_property_display_mode;
			Glib::PropertyProxy<LayerDisplay::Mode> property_display_mode() { return p_property_display_mode.get_proxy(); }
			typedef sigc::signal<void, const Glib::ustring&> type_signal_activate;
			type_signal_activate signal_activate() {return s_signal_activate;}


		protected:


			virtual void render_vfunc( const Cairo::RefPtr<Cairo::Context>& cr,
									   Gtk::Widget& widget,
									   const Gdk::Rectangle& background_area,
									   const Gdk::Rectangle& cell_area,
									   Gtk::CellRendererState flags );

			virtual void get_preferred_width_vfunc(Gtk::Widget& widget,
												   int& min_w,
												   int& nat_w) const;

			virtual void get_preferred_height_vfunc(Gtk::Widget& widget,
													int& min_h,
													int& nat_h) const;

			virtual bool activate_vfunc(GdkEvent *event,
										Gtk::Widget &widget,
										const Glib::ustring &path,
										const Gdk::Rectangle &background_area,
										const Gdk::Rectangle &cell_area,
										Gtk::CellRendererState flags);

		private:
			type_property_color p_property_color;
			type_property_display_mode p_property_display_mode;
			type_signal_activate s_signal_activate;
	};

}
