#pragma once
#include "uuid.hpp"
#include "json_fwd.hpp"
#include "object.hpp"
#include "unit.hpp"
#include "block.hpp"
#include "sheet.hpp"
#include <vector>
#include <map>
#include <fstream>

namespace horizon {
	using json = nlohmann::json;

	class Schematic {
		private :
			Schematic(const UUID &uu, const json &, Block &block, Object &pool);
			unsigned int update_nets();


		public :
			static Schematic new_from_file(const std::string &filename, Block &block, Object &pool);
			Schematic(const UUID &uu, Block &block);
			void expand(bool careful=false);

			Schematic(const Schematic &sch);
			void operator=(const Schematic &sch);
			void update_refs();
			void merge_nets(Net *net, Net *into);
			void disconnect_symbol(Sheet *sheet, SchematicSymbol *sym);
			void autoconnect_symbol(Sheet *sheet, SchematicSymbol *sym);
			void smash_symbol(Sheet *sheet, SchematicSymbol *sym);
			void unsmash_symbol(Sheet *sheet, SchematicSymbol *sym);


			UUID uuid;
			Block *block;
			std::string name;
			std::map<const UUID, Sheet> sheets;

			class Annotation {
				public:
					Annotation(const json &j);
					Annotation();
					enum class Order {RIGHT_DOWN, DOWN_RIGHT};
					Order order = Order::RIGHT_DOWN;

					enum class Mode {SEQUENTIAL, SHEET_100, SHEET_1000};
					Mode mode = Mode::SHEET_100;

					bool fill_gaps = true;
					bool keep = true;
					json serialize() const;
			};

			Annotation annotation;
			void annotate();

			json serialize() const;
	};

}
