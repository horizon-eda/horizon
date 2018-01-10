#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common.hpp"
#include "uuid.hpp"
#include "uuid_path.hpp"
#include "pool.hpp"
namespace horizon {


	class CreatePartDialog: public Gtk::Dialog {
		public:
		CreatePartDialog(Gtk::Window *parent, Pool *ipool);
			UUID get_entity();
			UUID get_package();

		private :
			Pool *pool;
			class PoolBrowserEntity *browser_entity = nullptr;
			class PoolBrowserPackage *browser_package = nullptr;
			Gtk::Button *button_ok;
			void check_select();
			void check_activate();
	};
}
