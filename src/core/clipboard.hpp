#pragma once
#include <gtkmm.h>
#include "canvas/selectables.hpp"
#include "core.hpp"
#include "schematic/line_net.hpp"
#include "buffer.hpp"
#include <set>

namespace horizon {

	/**
	 * The ClipBoardManager handles the copy part of copy/paste.
	 * Contrary to other EDA packages, horizon uses the
	 * operating system's clipboard for copy/paste.
	 *
	 * When data is requested, the buffer gets serialized to json.
	 */
	class ClipboardManager {
		public:
			ClipboardManager(Core *co);
			/**
			 * Copys the objects specified by selection to the buffer.
			 * \param selection Which objects to copy
			 * \param cursor_pos Upon paste, objects will appear relativ to this point
			 */
			void copy(std::set<SelectableRef> selection, const Coordi &cursor_pos);

		private:
			void on_clipboard_get(Gtk::SelectionData& selection_data, guint /* info */);
			void on_clipboard_clear();
			Buffer buffer;
			Core *core;
			Coordi cursor_pos;
	};
}
