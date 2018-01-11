#pragma once
#include <gtkmm.h>
#include <utility>
#include "pool/symbol.hpp"

namespace horizon {
	class SymbolPreviewBox: public Gtk::Box  {
		public:
			SymbolPreviewBox(const std::pair<int, bool> &view);
			void update(const Symbol &sym);
			void zoom_to_fit();
			std::map<std::tuple<int, bool, UUID>, Placement> get_text_placements() const;
			void set_text_placements(const std::map<std::tuple<int, bool, UUID>, Placement> &p);

		private:
			class CanvasGL *canvas = nullptr;
			Gtk::Button *set_button = nullptr;
			const std::pair<int, bool> view;
			Symbol symbol;
			std::map<UUID, Placement> text_placements;
			void set_placements();
			void clear_placements();
	};
}
