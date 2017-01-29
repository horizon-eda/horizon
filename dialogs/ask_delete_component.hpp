#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common.hpp"
#include "uuid.hpp"
#include "component.hpp"
namespace horizon {


	class AskDeleteComponentDialog: public Gtk::Dialog {
		public:
		AskDeleteComponentDialog(Gtk::Window *parent, Component *comp);

			//virtual ~MainWindow();
		private :



	};
}
