#pragma once
#include <gtkmm.h>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "preview_base.hpp"

namespace horizon {
	class PartPreview: public Gtk::Box, public PreviewBase {
		public:
			PartPreview(class Pool &pool, bool show_goto=true);

			void load(const class Part *part);

		private:
			class Pool &pool;
			const class Part *part = nullptr;
			class EntityPreview *entity_preview = nullptr;

			class PreviewCanvas *canvas_package = nullptr;
			Gtk::ComboBoxText *combo_package = nullptr;

			Gtk::Label *label_MPN = nullptr;
			Gtk::Label *label_manufacturer = nullptr;
			Gtk::Label *label_value = nullptr;
			Gtk::Label *label_description = nullptr;
			Gtk::Label *label_datasheet = nullptr;
			Gtk::Label *label_entity = nullptr;

			void handle_package_sel();
	};
}
