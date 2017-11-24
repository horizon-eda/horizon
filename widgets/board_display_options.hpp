#pragma once
#include <gtkmm.h>
#include "canvas/layer_display.hpp"
#include "canvas/canvas.hpp"

namespace horizon {
	class BoardDisplayOptionsBox: public Gtk::Box {
		friend class LayerOptionsExp;
		public :
			BoardDisplayOptionsBox(class LayerProvider *lp);

			typedef sigc::signal<void, int, LayerDisplay> type_signal_set_layer_display;
			type_signal_set_layer_display signal_set_layer_display() {return s_signal_set_layer_display;}

			void update();

		private:
			class LayerProvider *lp;
			type_signal_set_layer_display s_signal_set_layer_display;

			Gtk::Expander *expander_all = nullptr;
			bool expanding = false;
			bool all_updating = false;
			void update_expand_all();

	};
}
