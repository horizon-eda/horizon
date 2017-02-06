#pragma once
#include "uuid.hpp"
#include "json_fwd.hpp"
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

	class Pool : public Object {
		public :
			Pool(const std::string &bp);
			virtual Unit *get_unit(const UUID &uu);
			virtual Entity *get_entity(const UUID &uu);
			virtual Symbol *get_symbol(const UUID &uu);
			virtual Padstack *get_padstack(const UUID &uu);
			virtual class Package *get_package(const UUID &uu);
			virtual class Part *get_part(const UUID &uu);
			SQLite::Database db;
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
