#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "block/net.hpp"
namespace horizon {


	class AskNetMergeDialog: public Gtk::Dialog {
		public:
			AskNetMergeDialog(Gtk::Window *parent, Net *a, Net *b);

			//virtual ~MainWindow();
		private :
			Net *net = nullptr;
			Net *into = nullptr;

			void ok_clicked();

	};
}
