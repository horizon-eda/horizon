#include <vector>
#include <string>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>
#include <giomm/file.h>
#include <giomm/resource.h>
#include <iostream>
#include <sstream>
#include "unit.hpp"
#include "sqlite.hpp"
#include "pool.hpp"
#include "package.hpp"
#include "part.hpp"

using json = nlohmann::json;

bool endswidth(const std::string &haystack, const std::string &needle) {
	return (haystack.size()-haystack.rfind(needle)) == needle.size();
}

void update_units(SQLite::Database &db, const std::string &directory, const std::string &prefix="") {
	if(prefix.size()==0) {
		db.execute("DELETE from units");
	}
	Glib::Dir dir(directory);
	for(const auto &it: dir) {
		std::string filename = Glib::build_filename(directory, it);
		if(endswidth(it, ".json")) {
			auto unit = horizon::Unit::new_from_file(filename);
			SQLite::Query q(db, "INSERT INTO units (uuid, name, filename) VALUES ($uuid, $name, $filename)");
			q.bind("$uuid", unit.uuid);
			q.bind("$name", unit.name);
			q.bind("$filename", Glib::build_filename(prefix, it));
			q.step();
		}
		else if(Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
			update_units(db, filename, Glib::build_filename(prefix, it));
		}
	}
}

void update_entities(SQLite::Database &db, horizon::Pool &pool, const std::string &directory, const std::string &prefix="") {
	if(prefix.size()==0) {
		db.execute("DELETE from entities");
	}
	Glib::Dir dir(directory);
	for(const auto &it: dir) {
		std::string filename = Glib::build_filename(directory, it);
		if(endswidth(it, ".json")) {
			auto entity = horizon::Entity::new_from_file(filename, pool);
			SQLite::Query q(db, "INSERT INTO entities (uuid, name, filename, n_gates, prefix) VALUES ($uuid, $name, $filename, $n_gates, $prefix)");
			q.bind("$uuid", entity.uuid);
			q.bind("$name", entity.name);
			q.bind("$n_gates", entity.gates.size());
			q.bind("$prefix", entity.prefix);
			q.bind("$filename", Glib::build_filename(prefix, it));
			q.step();
			for(const auto &it_tag: entity.tags) {
				SQLite::Query q2(db, "INSERT into tags (tag, uuid, type) VALUES ($tag, $uuid, 'entity')");
				q2.bind("$uuid", entity.uuid);
				q2.bind("$tag", it_tag);
				q2.step();
			}
		}
		else if(Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
			update_entities(db, pool, filename, Glib::build_filename(prefix,it));
		}
	}
}

void update_symbols(SQLite::Database &db, horizon::Pool &pool, const std::string &directory, const std::string &prefix="") {
	if(prefix.size() == 0) {
		db.execute("DELETE from symbols");
	}
	Glib::Dir dir(directory);
	for(const auto &it: dir) {
		std::string filename = Glib::build_filename(directory, it);
		if(endswidth(it, ".json")) {
			auto symbol = horizon::Symbol::new_from_file(filename, pool);
			SQLite::Query q(db, "INSERT INTO symbols (uuid, name, filename, unit) VALUES ($uuid, $name, $filename, $unit)");
			q.bind("$uuid", symbol.uuid);
			q.bind("$name", symbol.name);
			q.bind("$unit", symbol.unit->uuid);
			q.bind("$filename", Glib::build_filename(prefix, it));
			q.step();
		}
		else if(Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
			update_symbols(db, pool, filename, Glib::build_filename(prefix,it));
		}
	}
}

void update_padstacks(SQLite::Database &db, const std::string &directory, const std::string &prefix="") {
	if(prefix.size()==0) {
		db.execute("DELETE from padstacks");
	}
	Glib::Dir dir(directory);
	for(const auto &it: dir) {
		auto pkgpath = Glib::build_filename(directory, it);
		auto pkgfilename = Glib::build_filename(pkgpath, "package.json");
		if(Glib::file_test(pkgfilename, Glib::FileTest::FILE_TEST_IS_REGULAR)) {
			horizon::UUID pkg_uuid;
			//we'll have to parse the package manually, since we don't have padstacks yet
			{
				std::ifstream ifs(Glib::build_filename(pkgpath, "package.json"));
				json j;
				if(!ifs.is_open()) {
					throw std::runtime_error("package not opened");
				}
				ifs>>j;
				ifs.close();
				pkg_uuid = j.at("uuid").get<std::string>();
			}

			auto padstacks_path = Glib::build_filename(pkgpath, "padstacks");
			Glib::Dir dir2(padstacks_path);
			for(const auto &it2: dir2) {
				if(endswidth(it2, ".json")) {
					std::string filename = Glib::build_filename(padstacks_path, it2);
					auto padstack = horizon::Padstack::new_from_file(filename);
					SQLite::Query q(db, "INSERT INTO padstacks (uuid, name, filename, package) VALUES ($uuid, $name, $filename, $package)");
					q.bind("$uuid", padstack.uuid);
					q.bind("$name", padstack.name);
					q.bind("$package", pkg_uuid);
					q.bind("$filename", Glib::build_filename(prefix, it, "padstacks", it2));
					q.step();
				}
			}
		}
		else if(Glib::file_test(pkgpath, Glib::FILE_TEST_IS_DIR)) {
			update_padstacks(db, pkgpath, Glib::build_filename(prefix,it));
		}

	}
}

void update_packages(SQLite::Database &db, horizon::Pool &pool, const std::string &directory, const std::string &prefix="") {
	if(prefix.size()==0) {
		db.execute("DELETE from packages");
	}
	Glib::Dir dir(directory);
	for(const auto &it: dir) {
		auto pkgpath = Glib::build_filename(directory, it);
		auto pkgfilename = Glib::build_filename(pkgpath, "package.json");
		if(Glib::file_test(pkgfilename, Glib::FileTest::FILE_TEST_IS_REGULAR)) {
			std::string filename = Glib::build_filename(pkgpath, "package.json");
			auto package = horizon::Package::new_from_file(filename, pool);
			SQLite::Query q(db, "INSERT INTO packages (uuid, name, filename, n_pads) VALUES ($uuid, $name, $filename, $n_pads)");
			q.bind("$uuid", package.uuid);
			q.bind("$name", package.name);
			q.bind("$n_pads", package.pads.size());
			q.bind("$filename", Glib::build_filename(prefix, it, "package.json"));
			q.step();
			for(const auto &it_tag: package.tags) {
				SQLite::Query q2(db, "INSERT into tags (tag, uuid, type) VALUES ($tag, $uuid, 'package')");
				q2.bind("$uuid", package.uuid);
				q2.bind("$tag", it_tag);
				q2.step();
			}
		}
		else if(Glib::file_test(pkgpath, Glib::FILE_TEST_IS_DIR)) {
			update_packages(db, pool, pkgpath, Glib::build_filename(prefix,it));
		}
	}
}

bool update_parts(SQLite::Database &db, horizon::Pool &pool, const std::string &directory, const std::string &prefix="") {
	if(prefix.size()==0)
		db.execute("DELETE from parts");

	Glib::Dir dir(directory);
	bool skipped = true;
	while(skipped) {
		skipped = false;
		for(const auto &it: dir) {
			std::string filename = Glib::build_filename(directory, it);
			if(endswidth(it, ".json")) {
				bool skipthis = false;
				{
					std::ifstream ifs(filename);
					json j;
					if(!ifs.is_open()) {
						throw std::runtime_error("part not opened");
					}
					ifs>>j;
					ifs.close();
					if(j.count("base")) {
						horizon::UUID base_uuid = j.at("base").get<std::string>();
						SQLite::Query q(db, "SELECT uuid from parts WHERE uuid = $uuid");
						q.bind("$uuid", base_uuid);
						if(!q.step()) { //not found
							skipthis = true;
							skipped = true;
						}
					}
				}
				if(!skipthis) {
					db.execute("BEGIN TRANSACTION");
					auto part = horizon::Part::new_from_file(filename, pool);
					SQLite::Query q(db, "INSERT INTO parts (uuid, MPN, manufacturer, entity, package, filename) VALUES ($uuid, $MPN, $manufacturer, $entity, $package, $filename)");
					q.bind("$uuid", part.uuid);
					q.bind("$MPN", part.get_MPN());
					q.bind("$manufacturer", part.get_manufacturer());
					q.bind("$package", part.package->uuid);
					q.bind("$entity", part.entity->uuid);
					q.bind("$filename", Glib::build_filename(prefix, it));
					q.step();

					for(const auto &it_tag: part.get_tags()) {
						SQLite::Query q2(db, "INSERT into tags (tag, uuid, type) VALUES ($tag, $uuid, 'part')");
						q2.bind("$uuid", part.uuid);
						q2.bind("$tag", it_tag);
						q2.step();
					}
					db.execute("COMMIT");
				}
			}
			else if(Glib::file_test(filename, Glib::FILE_TEST_IS_DIR)) {
				if(update_parts(db, pool, filename, Glib::build_filename(prefix,it)))
					skipped = true;
			}
		}
		if(prefix.size()>0)
			break;
	}
	return skipped;

}

int main(int c_argc, char *c_argv[]) {
	std::vector<std::string> argv;
	for(int i = 0; i<c_argc; i++) {
		argv.emplace_back(c_argv[i]);
	}
	auto pool_base_path = Glib::getenv("HORIZON_POOL");
	auto pool_db_path = Glib::build_filename(pool_base_path, "pool.db");
	SQLite::Database db(pool_db_path, SQLITE_OPEN_CREATE|SQLITE_OPEN_READWRITE);
	{
		SQLite::Query q(db, "SELECT name FROM sqlite_master WHERE type='table' AND name='units'");
		if(!q.step()) { //db is likely empty
			auto bytes = Gio::Resource::lookup_data_global("/net/carrotIndustries/horizon/pool-update/schema.sql");
			gsize size {bytes->get_size()+1};//null byte
			auto data = (const char*)bytes->get_data(size);
			db.execute(data);
			std::cout << "created db from schema" << std::endl;
		}
	}


	horizon::Pool pool(pool_base_path);


	db.execute("BEGIN TRANSACTION");
	db.execute("DELETE FROM tags");
	update_units(db, Glib::build_filename(pool_base_path, "units"));
	db.execute("COMMIT");

	db.execute("BEGIN TRANSACTION");
	update_entities(db, pool, Glib::build_filename(pool_base_path, "entities"));
	db.execute("COMMIT");

	db.execute("BEGIN TRANSACTION");
	update_symbols(db, pool, Glib::build_filename(pool_base_path, "symbols"));
	db.execute("COMMIT");

	db.execute("BEGIN TRANSACTION");
	update_padstacks(db, Glib::build_filename(pool_base_path, "packages"));
	db.execute("COMMIT");

	db.execute("BEGIN TRANSACTION");
	update_packages(db, pool, Glib::build_filename(pool_base_path, "packages"));
	db.execute("COMMIT");

	update_parts(db, pool, Glib::build_filename(pool_base_path, "parts"));

	return 0;
}
