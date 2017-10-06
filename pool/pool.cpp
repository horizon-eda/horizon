#include "pool.hpp"
#include "padstack.hpp"
#include "package.hpp"
#include "part.hpp"
#include "unit.hpp"
#include "entity.hpp"
#include "symbol.hpp"
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>


namespace horizon {
	Pool::Pool(const std::string &bp) :db(bp+"/pool.db", SQLITE_OPEN_READONLY, 1000), base_path(bp)  {
	}

	void Pool::clear() {
		units.clear();
		symbols.clear();
		entities.clear();
		padstacks.clear();
		packages.clear();
		parts.clear();
	}


	std::string Pool::get_filename(ObjectType type, const UUID &uu) {
		std::string query;
		switch(type) {
			case ObjectType::UNIT :
				query = "SELECT filename FROM units WHERE uuid = ?";
			break;

			case ObjectType::ENTITY :
				query = "SELECT filename FROM entities WHERE uuid = ?";
			break;

			case ObjectType::SYMBOL :
				query = "SELECT filename FROM symbols WHERE uuid = ?";
			break;

			case ObjectType::PACKAGE :
				query = "SELECT filename FROM packages WHERE uuid = ?";
			break;

			case ObjectType::PADSTACK :
				query = "SELECT filename FROM padstacks WHERE uuid = ?";
			break;

			case ObjectType::PART :
				query = "SELECT filename FROM parts WHERE uuid = ?";
			break;

			default:
				return "";
		}
		SQLite::Query q(db, query);
		q.bind(1, uu);
		if(!q.step()) {
			auto tf = get_tmp_filename(type, uu);

			if(tf.size() && Glib::file_test(tf, Glib::FILE_TEST_IS_REGULAR))
				return tf;
			else
				throw std::runtime_error("not found");
		}
		auto filename = q.get<std::string>(0);
		switch(type) {
			case ObjectType::UNIT :
				return Glib::build_filename(base_path, "units", filename);

			case ObjectType::ENTITY :
				return Glib::build_filename(base_path, "entities", filename);

			case ObjectType::SYMBOL :
				return Glib::build_filename(base_path, "symbols", filename);

			case ObjectType::PACKAGE :
				return Glib::build_filename(base_path, "packages", filename);

			case ObjectType::PADSTACK :
				return Glib::build_filename(base_path, filename);

			case ObjectType::PART :
				return Glib::build_filename(base_path, "parts", filename);

			default:
				return "";
		}
	}

	const std::string &Pool::get_base_path() const {
		return base_path;
	}

	std::string Pool::get_tmp_filename(ObjectType type, const UUID &uu) const {
		auto suffix = static_cast<std::string>(uu)+".json";
		auto base = Glib::build_filename(base_path, "tmp");
		switch(type) {
			case ObjectType::UNIT :
				return Glib::build_filename(base, "unit_"+suffix);

			case ObjectType::ENTITY :
				return Glib::build_filename(base, "entity_"+suffix);

			case ObjectType::SYMBOL :
				return Glib::build_filename(base, "sym_"+suffix);

			case ObjectType::PACKAGE :
				return Glib::build_filename(base, "pkg_"+suffix);

			case ObjectType::PADSTACK :
				return Glib::build_filename(base, "ps_"+suffix);

			case ObjectType::PART :
				return Glib::build_filename(base, "part_"+suffix);

			default:
				return "";
		}
	}

	const Unit *Pool::get_unit(const UUID &uu) {
		if(units.count(uu) == 0) {
			std::string path = get_filename(ObjectType::UNIT, uu);
			Unit u = Unit::new_from_file(path);
			units.insert(std::make_pair(uu, u));
		}
		return &units.at(uu);
	}
	
	const Entity *Pool::get_entity(const UUID &uu) {
		if(entities.count(uu) == 0) {
			std::string path = get_filename(ObjectType::ENTITY, uu);
			Entity e = Entity::new_from_file(path, *this);
			entities.insert(std::make_pair(uu, e));
		}
		return &entities.at(uu);
	}

	const Symbol *Pool::get_symbol(const UUID &uu) {
		if(symbols.count(uu) == 0) {
			std::string path = get_filename(ObjectType::SYMBOL, uu);
			Symbol s = Symbol::new_from_file(path, *this);
			symbols.insert(std::make_pair(uu, s));
		}
		return &symbols.at(uu);
	}

	const Package *Pool::get_package(const UUID &uu) {
		if(packages.count(uu) == 0) {
			std::string path = get_filename(ObjectType::PACKAGE, uu);
			Package p = Package::new_from_file(path, *this);
			packages.emplace(uu, p);
		}
		return &packages.at(uu);
	}

	const Padstack *Pool::get_padstack(const UUID &uu) {
		if(padstacks.count(uu) == 0) {
			std::string path = get_filename(ObjectType::PADSTACK, uu);
			Padstack p = Padstack::new_from_file(path);
			padstacks.insert(std::make_pair(uu, p));
		}
		return &padstacks.at(uu);
	}

	const Part *Pool::get_part(const UUID &uu) {
		if(parts.count(uu) == 0) {
			std::string path = get_filename(ObjectType::PART, uu);
			Part p = Part::new_from_file(path, *this);
			parts.insert(std::make_pair(uu, p));
		}
		return &parts.at(uu);
	}


}
