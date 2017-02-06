#pragma once
#include "symbol.hpp"
#include "pool.hpp"
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

			Junction *get_junction(const UUID &uu, bool work=true);
			Line *get_line(const UUID &uu, bool work=true);
			SymbolPin *get_symbol_pin(const UUID &uu, bool work=true);
			Arc *get_arc(const UUID &uu, bool work=true);

			Junction *insert_junction(const UUID &uu, bool work=true);
			void delete_junction(const UUID &uu, bool work=true);
			Line *insert_line(const UUID &uu, bool work=true);
			void delete_line(const UUID &uu, bool work=true);
			Arc *insert_arc(const UUID &uu, bool work=true);
			void delete_arc(const UUID &uu, bool work=true);

			SymbolPin *insert_symbol_pin(const UUID &uu, bool work=true);
			void delete_symbol_pin(const UUID &uu, bool work=true);


			std::vector<Line*> get_lines(bool work=true);
			std::vector<Arc*> get_arcs(bool work=true);
			std::vector<Pin*> get_pins(bool work=true);

			void rebuild(bool from_undo=false);
			void commit();
			void revert();
			void save();

			std::string get_property_string(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled=nullptr);

			bool get_property_bool(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled=nullptr);
			void set_property_bool(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool value, bool *handled=nullptr);

			int64_t get_property_int(const UUID &uu, ObjectType type, ObjectProperty::ID property, bool *handled=nullptr);
			void set_property_int(const UUID &uu, ObjectType type, ObjectProperty::ID property, int64_t value, bool *handled=nullptr);

			const Symbol *get_canvas_data();
			std::pair<Coordi,Coordi> get_bbox() override;

		private:
			std::map<UUID, Text> *get_text_map(bool work=true) override;


			Symbol sym;
			Symbol sym_work;
			std::string m_filename;

			class HistoryItem: public Core::HistoryItem {
				public:
				HistoryItem(const Symbol &s):sym(s) {}
				Symbol sym;
			};
			void history_push();
			void history_load(unsigned int i);
	};
}
