#pragma once
#include <gtkmm.h>

namespace horizon {


	class ProjectManagerPrefs: public Gtk::Dialog {
		public:
			ProjectManagerPrefs(Gtk::ApplicationWindow *parent);
		private :
			Glib::RefPtr<class Gtk::Application> app;
			void update();
			class ProjectManagerPrefsBox *box;


	};

}
