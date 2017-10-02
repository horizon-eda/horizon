#pragma once
#include "imp.hpp"

namespace horizon {
	class ImpSymbol : public ImpBase {
		public :
			ImpSymbol(const std::string &symbol_filename, const std::string &pool_path);


		protected:
			void construct() override;
			ToolID handle_key(guint k) override;
		private:
			void canvas_update() override;
			CoreSymbol core_symbol;

			Gtk::Entry *name_entry = nullptr;
			class SymbolPreviewWindow *symbol_preview_window = nullptr;
	};
}
