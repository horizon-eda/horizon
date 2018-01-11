#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"

namespace horizon {
	class DuplicateEntityWidget: public Gtk::Box {
		public:
			DuplicateEntityWidget(class Pool *p, const UUID &entity_uuid, Gtk::Box *ubox, bool optional=false, class DuplicateWindow *w=nullptr);

			UUID duplicate();
			UUID get_uuid() const;

		private:
			class Pool *pool;
			const class Entity *entity;
			Gtk::Entry *name_entry  = nullptr;
			class LocationEntry *location_entry = nullptr;
			Gtk::Grid *grid = nullptr;
			Gtk::Box *unit_box = nullptr;

			class DuplicateWindow *win;
	};
}
