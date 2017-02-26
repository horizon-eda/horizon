#pragma once
#include "uuid.hpp"
#include "json.hpp"
#include "common.hpp"
#include "unit.hpp"
#include "entity.hpp"
#include "object.hpp"
#include "symbol.hpp"
#include "padstack.hpp"
#include <vector>
#include <map>
#include <fstream>
#include "sqlite.hpp"

namespace horizon {

	/**
	 * Stores objects (Unit, Entity, Symbol, Part, etc.) from the pool.
	 * Objects are lazy-loaded when they're accessed for the first time.
	 */
	class Pool : public Object {
		public :
			/**
			 * Constructs a Pool
			 * @param base_path Path to the pool containing the pool.db
			 */
			Pool(const std::string &base_path);
			virtual Unit *get_unit(const UUID &uu);
			virtual Entity *get_entity(const UUID &uu);
			virtual Symbol *get_symbol(const UUID &uu);
			virtual Padstack *get_padstack(const UUID &uu);
			virtual class Package *get_package(const UUID &uu);
			virtual class Part *get_part(const UUID &uu);
			/**
			 * The database connection.
			 * You may use it to perform more advanced queries on the pool.
			 */
			SQLite::Database db;
			/**
			 * Clears all lazy-loaded objects.
			 * Doing so will invalidate all references pointers by get_entity and friends.
			 */
			void clear();

		
		private :
			std::string base_path;

			std::map<UUID, Unit> units;
			std::map<UUID, Entity> entities;
			std::map<UUID, Symbol> symbols;
			std::map<UUID, Padstack> padstacks;
			std::map<UUID, Package> packages;
			std::map<UUID, Part> parts;

		
	};

}
