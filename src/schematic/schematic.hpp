#pragma once
#include "util/uuid.hpp"
#include "json.hpp"
#include "pool/unit.hpp"
#include "block/block.hpp"
#include "sheet.hpp"
#include "schematic_rules.hpp"
#include <vector>
#include <map>
#include <fstream>

namespace horizon {
	using json = nlohmann::json;

	/**
	 * A Schematic is the visual representation of a Block.
	 * Contrary to other EDA systems, the Schematic isn't the
	 * source of truth to horizon, the Block is. During editing,
	 * Block and Schematic are edited in sync. After Editing is done,
	 * Schematic::expand() updates the Schematic according to the Block,
	 * filling in reference designators and assigning nets to LineNets.
	 *
	 * A Schematic is made up of one or many Sheets.
	 *
	 */
	class Schematic {
		private :
			Schematic(const UUID &uu, const json &, Block &block, class Pool &pool);
			unsigned int update_nets();


		public :
			static Schematic new_from_file(const std::string &filename, Block &block, Pool &pool);
			Schematic(const UUID &uu, Block &block);

			/**
			 * This is where the magic happens.
			 * \param carful when true, superfluous things will get cleaned up. Don't do this when you may hold pointers to these.
			 */
			void expand(bool careful=false);

			Schematic(const Schematic &sch);
			void operator=(const Schematic &sch);
			/**
			 * objects owned by the Sheets may hold pointers to other objects of the same sheet
			 * or the Block associated with the Schematic. When pointing block to a different block, call this
			 * method to fix up pointers. When copying Schematic, the copy constructor will call this method.
			 */
			void update_refs();

			/**
			 * Removes all connections from sym and connects the dangling net lines to junctions
			 */
			void disconnect_symbol(Sheet *sheet, SchematicSymbol *sym);

			/**
			 * Connects unconnected pins of sym to Nets specified by junctions coincident with pins.
			 */
			void autoconnect_symbol(Sheet *sheet, SchematicSymbol *sym);

			/**
			 * Turns sym's texts to regular text objects.
			 */
			void smash_symbol(Sheet *sheet, SchematicSymbol *sym);

			/**
			 * Undoes what smash_symbol did.
			 */
			void unsmash_symbol(Sheet *sheet, SchematicSymbol *sym);


			UUID uuid;
			Block *block;
			std::string name;
			std::map<UUID, Sheet> sheets;
			SchematicRules rules;


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
