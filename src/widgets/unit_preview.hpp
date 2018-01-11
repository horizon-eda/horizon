#pragma once
#include <gtkmm.h>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "preview_base.hpp"

namespace horizon {
	class UnitPreview: public Gtk::Box, public PreviewBase {
		public:
			UnitPreview(class Pool &pool);

			void load(const class Unit *unit);

		private:
			class Pool &pool;
			const class Unit *unit = nullptr;
			class PreviewCanvas *canvas_symbol = nullptr;
			Gtk::ComboBoxText *combo_symbol = nullptr;

			void handle_symbol_sel();
	};
}
