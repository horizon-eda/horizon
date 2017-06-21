#pragma once
#include <gtkmm.h>
#include "uuid.hpp"

namespace horizon {
	class PadstackButton: public Gtk::Button {
		public:
			PadstackButton(class Pool &p, const UUID &pkg_uuid);
			Glib::PropertyProxy<horizon::UUID> property_selected_uuid() { return p_property_selected_uuid.get_proxy(); }

		protected :
			Glib::Property<UUID> p_property_selected_uuid;
			Pool &pool;
			UUID package_uuid;

			void on_clicked() override;
			void update_label();
	};

	class ViaPadstackButton: public Gtk::Button {
		public:
			ViaPadstackButton(class ViaPadstackProvider &vpp);
			Glib::PropertyProxy<horizon::UUID> property_selected_uuid() { return p_property_selected_uuid.get_proxy(); }

		protected :
			Glib::Property<UUID> p_property_selected_uuid;
			class ViaPadstackProvider &via_padstack_provider;

			void on_clicked() override;
			void update_label();
	};


}
