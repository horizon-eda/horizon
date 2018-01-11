#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "json.hpp"
#include "pool/pool_cached.hpp"
#include "pool/pool.hpp"

namespace horizon {
	using json = nlohmann::json;

	class PoolCacheWindow: public Gtk::Window {
		public:
			PoolCacheWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x, const std::string &cache_path, const std::string &pool_path);
			static PoolCacheWindow* create(Gtk::Window *p, const std::string &cache_path, const std::string &pool_path);

			void refresh_list();

		private :
			void selection_changed();
			void update_from_pool();

			std::string cache_path;
			std::string base_path;

			PoolCached pool_cached;
			Pool pool;

			Gtk::TreeView *pool_item_view = nullptr;
			Gtk::Stack *stack = nullptr;
			Gtk::TextView *delta_text_view = nullptr;
			Gtk::Button *update_from_pool_button = nullptr;
			Gtk::Label *status_label = nullptr;

			enum class ItemState {CURRENT, OUT_OF_DATE, MISSING_IN_POOL};

			class TreeColumns : public Gtk::TreeModelColumnRecord {
				public:
					TreeColumns() {
						Gtk::TreeModelColumnRecord::add( name ) ;
						Gtk::TreeModelColumnRecord::add( type ) ;
						Gtk::TreeModelColumnRecord::add( uuid ) ;
						Gtk::TreeModelColumnRecord::add( state ) ;
						Gtk::TreeModelColumnRecord::add( delta ) ;
						Gtk::TreeModelColumnRecord::add( filename_cached ) ;
					}
					Gtk::TreeModelColumn<Glib::ustring> name;
					Gtk::TreeModelColumn<std::string> filename_cached;
					Gtk::TreeModelColumn<ObjectType> type;
					Gtk::TreeModelColumn<UUID> uuid;
					Gtk::TreeModelColumn<ItemState> state;
					Gtk::TreeModelColumn<json> delta;
			} ;
			TreeColumns tree_columns;

			Glib::RefPtr<Gtk::ListStore> item_store;
	};
}
