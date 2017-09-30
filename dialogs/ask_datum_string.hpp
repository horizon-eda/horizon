#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
namespace horizon {


	class AskDatumStringDialog: public Gtk::Dialog {
		public:
			AskDatumStringDialog(Gtk::Window *parent, const std::string &label);
			Gtk::Entry *entry = nullptr;;

			//virtual ~MainWindow();
		private :



	};
}
