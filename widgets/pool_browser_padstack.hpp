#pragma once

#include "pool_browser.hpp"

namespace horizon {
	class PoolBrowserPadstack: public PoolBrowser {
		public:
			PoolBrowserPadstack(class Pool *p);
			void search() override;
			void set_package_uuid(const UUID &uu);

		protected:
			Glib::RefPtr<Gtk::ListStore> create_list_store() override;
			void create_columns() override;
			void add_sort_controller_columns() override;
			UUID uuid_from_row(const Gtk::TreeModel::Row &row) override;

		private:
			class ListColumns : public Gtk::TreeModelColumnRecord {
				public:
					ListColumns() {
						Gtk::TreeModelColumnRecord::add( padstack_name ) ;
						Gtk::TreeModelColumnRecord::add( padstack_type ) ;
						Gtk::TreeModelColumnRecord::add( package_name ) ;
						Gtk::TreeModelColumnRecord::add( uuid ) ;
					}
					Gtk::TreeModelColumn<Glib::ustring> padstack_name;
					Gtk::TreeModelColumn<Glib::ustring> padstack_type;
					Gtk::TreeModelColumn<Glib::ustring> package_name;
					Gtk::TreeModelColumn<UUID> uuid;
			} ;
			ListColumns list_columns;

			Gtk::Entry *name_entry = nullptr;
			UUID package_uuid;
	};

}
