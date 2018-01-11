#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "block/component.hpp"
namespace horizon {


	class AskDeleteComponentDialog: public Gtk::Dialog {
		public:
		AskDeleteComponentDialog(Gtk::Window *parent, Component *comp);

			//virtual ~MainWindow();
		private :



	};
}
