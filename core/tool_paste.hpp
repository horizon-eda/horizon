#pragma once
#include "core.hpp"
#include <gtkmm.h>

namespace horizon {

	class ToolPaste : public ToolBase {
		public :
		ToolPaste(Core *c, ToolID tid);
			virtual ToolResponse begin(const ToolArgs &args);
			virtual ToolResponse update(const ToolArgs &args);

		private:
			void on_clipboard_received(const Gtk::SelectionData& selection_data);
			Glib::RefPtr<Gtk::Clipboard> ref_clipboard;
			void fix_layer(int &la);
			void apply_shift(Coordi &c, const Coordi &cursor_pos);
			Coordi shift;
			bool shift_set = false;

	};
}
