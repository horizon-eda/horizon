#include "pool.hpp"
#include "padstack.hpp"
#include "package.hpp"
#include "part.hpp"
#include "unit.hpp"
#include "entity.hpp"
#include "symbol.hpp"

namespace horizon {
	Pool::Pool(const std::string &bp) :db(bp+"/pool.db", SQLITE_OPEN_READONLY), base_path(bp)  {
	}

	void Pool::clear() {
		units.clear();
	}

	const Unit *Pool::get_unit(const UUID &uu) {
		if(units.count(uu) == 0) {
			SQLite::Query q(db, "SELECT filename FROM units WHERE uuid = ?");
			q.bind(1, uu);
			if(!q.step()) {
				throw std::runtime_error("unit not found");
			}
			std::string path = base_path+"/units/"+q.get<std::string>(0);
			Unit u = Unit::new_from_file(path);
			units.insert(std::make_pair(uu, u));
		}
		return &units.at(uu);
	}
	
	const Entity *Pool::get_entity(const UUID &uu) {
		if(entities.count(uu) == 0) {
			SQLite::Query q(db, "SELECT filename FROM entities WHERE uuid = ?");
			q.bind(1, uu);
			if(!q.step()) {
				throw std::runtime_error("entity not found");
			}
			std::string path = base_path+"/entities/"+q.get<std::string>(0);
			Entity e = Entity::new_from_file(path, *this);
			entities.insert(std::make_pair(uu, e));
		}
		return &entities.at(uu);
	}

	const Symbol *Pool::get_symbol(const UUID &uu) {
		if(symbols.count(uu) == 0) {
			SQLite::Query q(db, "SELECT filename FROM symbols WHERE uuid = ?");
			q.bind(1, uu);
			if(!q.step()) {
				throw std::runtime_error("symbol not found");
			}
			std::string path = base_path+"/symbols/"+q.get<std::string>(0);
			Symbol s = Symbol::new_from_file(path, *this);
			symbols.insert(std::make_pair(uu, s));
		}
		return &symbols.at(uu);
	}

	const Package *Pool::get_package(const UUID &uu) {
		if(packages.count(uu) == 0) {
			SQLite::Query q(db, "SELECT filename FROM packages WHERE uuid = ?");
			q.bind(1, uu);
			if(!q.step()) {
				throw std::runtime_error("package not found");
			}
			std::string path = base_path+"/packages/"+q.get<std::string>(0);
			Package p = Package::new_from_file(path, *this);
			packages.emplace(uu, p);
		}
		return &packages.at(uu);
	}

	const Padstack *Pool::get_padstack(const UUID &uu) {
		if(padstacks.count(uu) == 0) {
			SQLite::Query q(db, "SELECT filename FROM padstacks WHERE uuid = ?");
			q.bind(1, uu);
			if(!q.step()) {
				throw std::runtime_error("padstack not found");
			}
			std::string path = base_path+"/"+q.get<std::string>(0);
			Padstack p = Padstack::new_from_file(path);
			padstacks.insert(std::make_pair(uu, p));
		}
		return &padstacks.at(uu);
	}

	const Part *Pool::get_part(const UUID &uu) {
		if(parts.count(uu) == 0) {
			SQLite::Query q(db, "SELECT filename FROM parts WHERE uuid = ?");
			q.bind(1, uu);
			if(!q.step()) {
				throw std::runtime_error("part not found");
			}
			std::string path = base_path+"/parts/"+q.get<std::string>(0);
			Part p = Part::new_from_file(path, *this);
			parts.insert(std::make_pair(uu, p));
		}
		return &parts.at(uu);
	}


}
