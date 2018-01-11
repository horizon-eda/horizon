#pragma once
#include "pool/symbol.hpp"
#include "pool/pool.hpp"
#include "core.hpp"
#include "schematic/schematic.hpp"
#include "block/block.hpp"
#include <memory>
#include <iostream>

namespace horizon {
	class CoreSchematic: public Core {
		public:
			CoreSchematic(const std::string &schematic_filename, const std::string &block_filename, Pool &pool);
			bool has_object_type(ObjectType ty) override;

			Junction *get_junction(const UUID &uu, bool work = true) override;
			Line *get_line(const UUID &uu, bool work = true) override;
			Arc *get_arc(const UUID &uu, bool work = true) override;
			SchematicSymbol *get_schematic_symbol(const UUID &uu, bool work=true);
			Schematic *get_schematic(bool work=true);
			Sheet *get_sheet(bool work=true);
			Text *get_text(const UUID &uu, bool work=true) override;

			Junction *insert_junction(const UUID &uu, bool work = true) override;
			void delete_junction(const UUID &uu, bool work = true) override;
			Line *insert_line(const UUID &uu, bool work = true) override;
			void delete_line(const UUID &uu, bool work = true) override;
			Arc *insert_arc(const UUID &uu, bool work = true) override;
			void delete_arc(const UUID &uu, bool work = true) override;
			SchematicSymbol *insert_schematic_symbol(const UUID &uu, const Symbol *sym, bool work = true);
			void delete_schematic_symbol(const UUID &uu, bool work = true);

			LineNet *insert_line_net(const UUID &uu, bool work = true);
			void delete_line_net(const UUID &uu, bool work = true);

			Text *insert_text(const UUID &uu, bool work = true) override;
			void delete_text(const UUID &uu, bool work = true) override;

			std::vector<Line*> get_lines(bool work = true) override;
			std::vector<Arc*> get_arcs(bool work = true) override;
			std::vector<LineNet*> get_net_lines(bool work=true);
			std::vector<NetLabel*> get_net_labels(bool work=true);

			class Block *get_block(bool work=true) override;
			class LayerProvider *get_layer_provider() override;

			bool set_property(ObjectType type, const UUID &uu, ObjectProperty::ID property, const class PropertyValue &value) override;
			bool get_property(ObjectType type, const UUID &uu, ObjectProperty::ID property, class PropertyValue &value) override;
			bool get_property_meta(ObjectType type, const UUID &uu, ObjectProperty::ID property, class PropertyMeta &meta) override;

			std::string get_display_name(ObjectType type, const UUID &uu) override;

			class Rules *get_rules() override;

			void rebuild(bool from_undo=false) override;
			void commit() override;
			void revert() override;
			void save() override;

			void add_sheet();
			void delete_sheet(const UUID &uu);

			void set_sheet(const UUID &uu);
			const Sheet *get_canvas_data();
			std::pair<Coordi,Coordi> get_bbox() override;

		private:
			Block block;

			Schematic sch;

			SchematicRules rules;

			UUID sheet_uuid;
			std::string m_schematic_filename;
			std::string m_block_filename;

			class HistoryItem: public Core::HistoryItem {
				public:
				HistoryItem(const Block &b, const Schematic &s):block(b), sch(s) {}
				Block block;
				Schematic sch;
			};
			void history_push() override;
			void history_load(unsigned int i) override;
	};
}
