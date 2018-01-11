#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "block/net.hpp"
#include "widgets/net_selector.hpp"
namespace horizon {


	class SelectNetDialog: public Gtk::Dialog {
		public:
			SelectNetDialog(Gtk::Window *parent, Block *b, const std::string &ti);
			bool valid = false;
			UUID net;
			NetSelector *net_selector;


		private :
			Block *block = nullptr;


			void ok_clicked();
			void net_selected(const UUID &uu);
	};
}
