#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common.hpp"
#include "uuid.hpp"
#include "uuid_path.hpp"
#include "pool.hpp"

namespace horizon {
	class PoolBrowserDialogPart: public Gtk::Dialog {
		public:
		PoolBrowserDialogPart(Gtk::Window *parent, Pool *pool, const UUID &entity_uuid);
			UUID selected_uuid;
			bool selection_valid = false;
			void set_MPN(const std::string &MPN);
			void set_show_none(bool v);
			//virtual ~MainWindow();
		private :
			class PoolBrowserPart *browser;

			void ok_clicked();
	};
}
