#pragma once

#include "pool_browser.hpp"

namespace horizon {
	class PoolBrowserEntity: public PoolBrowser {
		public:
			PoolBrowserEntity(class Pool *p);
			void search() override;
			ObjectType get_type() const override {return ObjectType::ENTITY;}

		protected:
			Glib::RefPtr<Gtk::ListStore> create_list_store() override;
			void create_columns() override;
			void add_sort_controller_columns() override;
			UUID uuid_from_row(const Gtk::TreeModel::Row &row) override;

		private:
			class ListColumns : public Gtk::TreeModelColumnRecord {
				public:
					ListColumns() {
						Gtk::TreeModelColumnRecord::add( entity_name ) ;
						Gtk::TreeModelColumnRecord::add( entity_manufacturer ) ;
						Gtk::TreeModelColumnRecord::add( prefix ) ;
						Gtk::TreeModelColumnRecord::add( n_gates ) ;
						Gtk::TreeModelColumnRecord::add( uuid ) ;
						Gtk::TreeModelColumnRecord::add( tags ) ;
						Gtk::TreeModelColumnRecord::add( path ) ;
					}
					Gtk::TreeModelColumn<Glib::ustring> entity_name;
					Gtk::TreeModelColumn<Glib::ustring> entity_manufacturer;
					Gtk::TreeModelColumn<Glib::ustring> prefix;
					Gtk::TreeModelColumn<Glib::ustring> tags;
					Gtk::TreeModelColumn<Glib::ustring> path;
					Gtk::TreeModelColumn<UUID> uuid;
					Gtk::TreeModelColumn<unsigned int> n_gates;
			} ;
			ListColumns list_columns;
			Gtk::Entry *name_entry = nullptr;
			Gtk::Entry *tags_entry = nullptr;


	};

}
