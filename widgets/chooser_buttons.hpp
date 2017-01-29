#pragma once
#include <gtkmm.h>
#include "uuid.hpp"
#include "pool.hpp"

namespace horizon {
	class PadstackButton: public Gtk::Button {
		public:
			PadstackButton(Pool &p, const UUID &pkg_uuid);
			Glib::PropertyProxy<horizon::UUID> property_selected_uuid() { return p_property_selected_uuid.get_proxy(); }

		protected :
			Glib::Property<UUID> p_property_selected_uuid;
			Pool &pool;
			UUID package_uuid;

			void on_clicked() override;
			void update_label();
	};


}
