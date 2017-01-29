#pragma once
#include <gtkmm.h>
#include "canvas/selectables.hpp"
#include "core.hpp"
#include "buffer.hpp"
#include <set>

namespace horizon {
	class ClipboardManager {
		public:
			ClipboardManager(Core *co);
			void copy(std::set<SelectableRef> selection, const Coordi &cursor_pos);

		private:
			void on_clipboard_get(Gtk::SelectionData& selection_data, guint /* info */);
			void on_clipboard_clear();
			Buffer buffer;
			Core *core;
			Coordi cursor_pos;
	};
}
