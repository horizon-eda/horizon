#pragma once
#include <gtkmm.h>
#include <set>
#include "common.hpp"
#include "uuid.hpp"
#include "preview_base.hpp"

namespace horizon {
	class EntityPreview: public Gtk::Box, public PreviewBase {
		public:
			EntityPreview(class Pool &pool);

			void load(const class Entity *entity);

		private:
			class Pool &pool;
			const class Entity *entity = nullptr;
			class PreviewCanvas *canvas_symbol = nullptr;
			Gtk::ComboBoxText *combo_gate = nullptr;
			Gtk::ComboBoxText *combo_symbol = nullptr;

			void handle_gate_sel();
			void handle_symbol_sel();

	};
}
