#pragma once
#include <gtkmm.h>
#include "common.hpp"
#include "unit.hpp"
#include "part.hpp"
#include "entity.hpp"
#include "../pool_notebook.hpp" //for processes

namespace horizon {

	class PartWizard: public Gtk::Window {
		friend class PadEditor;
		friend class GateEditorWizard;
		public:
			PartWizard(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x, const class Package *p, const std::string &bp, class Pool *po);
			static PartWizard* create(const class Package *p, const std::string &pool_base_path, class Pool *po);
			bool get_has_finished() const;

			~PartWizard();

		private :
			const class Package *pkg;
			std::string pool_base_path;
			class Pool *pool;

			Gtk::Button *button_back = nullptr;
			Gtk::Button *button_next = nullptr;
			Gtk::Button *button_finish = nullptr;
			Gtk::Stack *stack = nullptr;

			Gtk::ListBox *pads_lb = nullptr;
			Gtk::ToolButton *button_link_pads = nullptr;
			Gtk::ToolButton *button_unlink_pads = nullptr;
			Gtk::ToolButton *button_import_pads = nullptr;

			Glib::RefPtr<Gtk::SizeGroup> sg_name;
			Gtk::Box *page_assign = nullptr;
			Gtk::Box *page_edit = nullptr;
			Gtk::Box *edit_left_box = nullptr;

			Gtk::Entry *entity_name_entry = nullptr;
			Gtk::Entry *entity_prefix_entry = nullptr;
			Gtk::Entry *entity_tags_entry = nullptr;

			Gtk::Entry *part_mpn_entry = nullptr;
			Gtk::Entry *part_value_entry = nullptr;
			Gtk::Entry *part_manufacturer_entry = nullptr;
			Gtk::Entry *part_tags_entry = nullptr;

			class LocationEntry *entity_location_entry = nullptr;
			class LocationEntry *part_location_entry = nullptr;

			Part part;
			Entity entity;

			class ListColumns : public Gtk::TreeModelColumnRecord {
				public:
					ListColumns() {
						Gtk::TreeModelColumnRecord::add( name ) ;
					}
					Gtk::TreeModelColumn<Glib::ustring> name;
			} ;
			ListColumns list_columns;

			Glib::RefPtr<Gtk::ListStore> gate_name_store;
			void update_gate_names();
			void update_pin_warnings();
			std::map<std::pair<std::string, std::string>, std::set<class PadEditor*>> get_pin_names();
			void handle_link();
			void handle_unlink();
			void handle_import();
			void update_part();
			void import_pads(const json &j);
			void create_pad_editors();
			void autolink_pads();
			void link_pads(const std::deque<class PadEditor*> &eds);
			bool frozen = true;

			enum class Mode {ASSIGN, EDIT};

			void handle_next();
			void handle_back();
			void handle_finish();
			void finish();
			bool has_finished = false;

			Mode mode = Mode::ASSIGN;
			void set_mode(Mode mo);
			void prepare_edit();
			std::map<std::string, Unit> units;
			std::map<UUID, UUID> symbols;

			void spawn(PoolManagerProcess::Type type, const std::vector<std::string> &args);
			std::map<std::string, PoolManagerProcess> processes;
	};
}
