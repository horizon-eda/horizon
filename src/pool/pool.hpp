#pragma once
#include "util/uuid.hpp"
#include "json.hpp"
#include "common/common.hpp"
#include <vector>
#include <map>
#include <fstream>
#include <set>

#include "util/sqlite.hpp"

namespace horizon {

	/**
	 * Stores objects (Unit, Entity, Symbol, Part, etc.) from the pool.
	 * Objects are lazy-loaded when they're accessed for the first time.
	 */
	class Pool {
		public :
			/**
			 * Constructs a Pool
			 * @param base_path Path to the pool containing the pool.db
			 */
			Pool(const std::string &base_path);
			const class Unit *get_unit(const UUID &uu);
			const class Entity *get_entity(const UUID &uu);
			const class Symbol *get_symbol(const UUID &uu);
			const class Padstack *get_padstack(const UUID &uu);
			const class Package *get_package(const UUID &uu);
			const class Part *get_part(const UUID &uu);
			std::set<UUID> get_alternate_packages(const UUID &uu);
			std::string get_3d_model_filename(const UUID &package_uuid);

			virtual std::string get_filename(ObjectType type, const UUID &uu);
			const std::string &get_base_path() const;
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
			std::string get_tmp_filename(ObjectType type, const UUID &uu) const;
			static int get_required_schema_version();
			virtual ~Pool();

		
		protected :
			std::string base_path;
			std::string get_flat_filename(ObjectType type, const UUID &uu) const;


			std::map<UUID, Unit> units;
			std::map<UUID, Entity> entities;
			std::map<UUID, Symbol> symbols;
			std::map<UUID, Padstack> padstacks;
			std::map<UUID, Package> packages;
			std::map<UUID, Part> parts;

		
	};

}
