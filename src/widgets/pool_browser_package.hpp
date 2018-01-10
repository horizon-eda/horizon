#pragma once

#include "pool_browser.hpp"

namespace horizon {
	class PoolBrowserPackage: public PoolBrowser {
		public:
			PoolBrowserPackage(class Pool *p);
			void search() override;
			ObjectType get_type() const override {return ObjectType::PACKAGE;}

		protected:
			Glib::RefPtr<Gtk::ListStore> create_list_store() override;
			void create_columns() override;
			void add_sort_controller_columns() override;
			UUID uuid_from_row(const Gtk::TreeModel::Row &row) override;

		private:
			class ListColumns : public Gtk::TreeModelColumnRecord {
				public:
					ListColumns() {
						Gtk::TreeModelColumnRecord::add( name ) ;
						Gtk::TreeModelColumnRecord::add( manufacturer) ;
						Gtk::TreeModelColumnRecord::add( n_pads ) ;
						Gtk::TreeModelColumnRecord::add( tags ) ;
						Gtk::TreeModelColumnRecord::add( uuid ) ;
						Gtk::TreeModelColumnRecord::add( path ) ;
					}
					Gtk::TreeModelColumn<Glib::ustring> name;
					Gtk::TreeModelColumn<Glib::ustring> manufacturer;
					Gtk::TreeModelColumn<Glib::ustring> tags;
					Gtk::TreeModelColumn<Glib::ustring> path;
					Gtk::TreeModelColumn<unsigned int> n_pads;
					Gtk::TreeModelColumn<UUID> uuid;
			} ;
			ListColumns list_columns;
			Gtk::Entry *name_entry = nullptr;
			Gtk::Entry *tags_entry = nullptr;
	};

}
