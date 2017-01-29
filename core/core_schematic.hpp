#pragma once
#include "symbol.hpp"
#include "pool.hpp"
#include "core.hpp"
#include "schematic.hpp"
#include "block.hpp"
#include "constraints.hpp"
#include <memory>
#include <iostream>

namespace horizon {
	class CoreSchematic: public Core {
		public:
			CoreSchematic(const std::string &schematic_filename, const std::string &block_filename, const std::string &constraints_filename, Pool &pool);
			bool has_object_type(ObjectType ty) override;

			virtual Junction *get_junction(const UUID &uu, bool work = true);
			virtual Line *get_line(const UUID &uu, bool work = true);
			virtual Arc *get_arc(const UUID &uu, bool work = true);
			virtual SchematicSymbol *get_schematic_symbol(const UUID &uu, bool work=true);
			virtual Schematic *get_schematic(bool work=true);
			virtual Sheet *get_sheet(bool work=true);
			virtual Text *get_text(const UUID &uu, bool work=true);

			virtual Junction *insert_junction(const UUID &uu, bool work = true);
			virtual void delete_junction(const UUID &uu, bool work = true);
			virtual Line *insert_line(const UUID &uu, bool work = true);
			virtual void delete_line(const UUID &uu, bool work = true);
			virtual Arc *insert_arc(const UUID &uu, bool work = true);
			virtual void delete_arc(const UUID &uu, bool work = true);
			virtual SchematicSymbol *insert_schematic_symbol(const UUID &uu, Symbol *sym, bool work = true);
			virtual void delete_schematic_symbol(const UUID &uu, bool work = true);

			virtual LineNet *insert_line_net(const UUID &uu, bool work = true);
			virtual void delete_line_net(const UUID &uu, bool work = true);

			virtual Text *insert_text(const UUID &uu, bool work = true);
			virtual void delete_text(const UUID &uu, bool work = true);

			virtual std::vector<Line*> get_lines(bool work = true);
			virtual std::vector<Arc*> get_arcs(bool work = true);
			virtual std::vector<LineNet*> get_net_lines(bool work=true);
			virtual std::vector<NetLabel*> get_net_labels(bool work=true);

			virtual bool property_is_settable(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled=nullptr);

			virtual std::string get_property_string(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled=nullptr);
			virtual void set_property_string(const UUID &uu, ObjectType type, ObjectProperty::ID property, const std::string &value, bool *handled=nullptr);

			virtual int64_t get_property_int(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled=nullptr);
			virtual void set_property_int(const UUID &uu, ObjectType type, ObjectProperty::ID property, int64_t value, bool *handled=nullptr);

			virtual bool get_property_bool(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled=nullptr);
			virtual void set_property_bool(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool value, bool *handled=nullptr);

			virtual void rebuild(bool from_undo=false);
			virtual void commit();
			virtual void revert();
			virtual void save();

			void add_sheet();
			void delete_sheet(const UUID &uu);

			Constraints *get_constraints() override;

			void set_sheet(const UUID &uu);
			const Sheet *get_canvas_data();
			std::pair<Coordi,Coordi> get_bbox() override;

		private:
			Constraints constraints;
			Block block;
			Block block_work;

			Schematic sch;
			Schematic sch_work;

			UUID sheet_uuid;
			std::string m_schematic_filename;
			std::string m_block_filename;
			std::string m_constraints_filename;

			class HistoryItem: public Core::HistoryItem {
				public:
				HistoryItem(const Block &b, const Schematic &s):block(b), sch(s) {}
				Block block;
				Schematic sch;
			};
			void history_push();
			void history_load(unsigned int i);
	};
}
