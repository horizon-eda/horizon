#pragma once
#include "pool/symbol.hpp"
#include "pool/pool.hpp"
#include "core.hpp"
#include <memory>
#include <iostream>
#include <deque>

namespace horizon {
	class CoreSymbol: public Core {
		public:
			CoreSymbol(const std::string &filename, Pool &pool);
			bool has_object_type(ObjectType ty) override;
			Symbol *get_symbol(bool work = true);

			Junction *get_junction(const UUID &uu, bool work=true) override;
			Line *get_line(const UUID &uu, bool work=true) override;
			SymbolPin *get_symbol_pin(const UUID &uu, bool work=true);
			Arc *get_arc(const UUID &uu, bool work=true) override;

			Junction *insert_junction(const UUID &uu, bool work=true) override;
			void delete_junction(const UUID &uu, bool work=true) override;
			Line *insert_line(const UUID &uu, bool work=true) override;
			void delete_line(const UUID &uu, bool work=true) override;
			Arc *insert_arc(const UUID &uu, bool work=true) override;
			void delete_arc(const UUID &uu, bool work=true) override;

			SymbolPin *insert_symbol_pin(const UUID &uu, bool work=true);
			void delete_symbol_pin(const UUID &uu, bool work=true);

			class LayerProvider *get_layer_provider() override;

			std::vector<Line*> get_lines(bool work=true) override;
			std::vector<Arc*> get_arcs(bool work=true) override;
			std::vector<const Pin*> get_pins(bool work=true);

			void rebuild(bool from_undo=false) override;
			void commit() override;
			void revert() override;
			void save() override;

			bool set_property(ObjectType type, const UUID &uu, ObjectProperty::ID property, const class PropertyValue &value) override;
			bool get_property(ObjectType type, const UUID &uu, ObjectProperty::ID property, class PropertyValue &value) override;
			bool get_property_meta(ObjectType type, const UUID &uu, ObjectProperty::ID property, class PropertyMeta &meta) override;

			std::string get_display_name(ObjectType type, const UUID &uu) override;

			const Symbol *get_canvas_data();
			std::pair<Coordi,Coordi> get_bbox() override;

		private:
			std::map<UUID, Text> *get_text_map(bool work=true) override;
			std::map<UUID, Polygon> *get_polygon_map(bool work=true) override;


			Symbol sym;
			std::string m_filename;

			class HistoryItem: public Core::HistoryItem {
				public:
				HistoryItem(const Symbol &s):sym(s) {}
				Symbol sym;
			};
			void history_push() override;
			void history_load(unsigned int i) override;
	};
}
