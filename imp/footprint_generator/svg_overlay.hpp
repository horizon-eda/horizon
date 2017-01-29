#pragma once
#include <gtkmm.h>
#include <librsvg/rsvg.h>

namespace horizon {
	class SVGOverlay: public Gtk::Overlay {
		public:
			SVGOverlay(const guint8 *data, gsize data_len);
			SVGOverlay(const char *resource);
			~SVGOverlay();

			void add_at_sub(Gtk::Widget& widget, const char *sub);
			std::map<std::string, std::string> sub_texts;

		private:
			RsvgHandle *handle=nullptr;
			void init(const guint8 *data, gsize data_len);
			Gtk::DrawingArea *area;
			bool draw(const Cairo::RefPtr<Cairo::Context> &ctx);


	};
}
