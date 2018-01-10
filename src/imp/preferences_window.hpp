#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
namespace horizon {

	class ImpPreferencesWindow: public Gtk::Window {
		friend class CanvasPreferencesEditor;
		public:
			ImpPreferencesWindow(class ImpPreferences *pr);

		private :
			class ImpPreferences *preferences;
	};
}
